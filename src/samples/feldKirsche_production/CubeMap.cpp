/*
 *  CubeMap.cpp
 *  CubeMapping
 *
 *  Created by David Wicks on 2/27/09.
 *  Copyright 2009 The Barbarian Group. All rights reserved.
 *
 */

#ifndef	CUBE_MAP_H
#define CUBE_MAP_H

#include "CubeMap.h"
#include "kinskiCore/file_functions.h"
#include "kinskiGL/Texture.h"


namespace kinski{ namespace gl{
    
    void copyMat(const MiniMat &src_mat, MiniMat &dst_mat);

    CubeMap create_cube_map(const std::string &path_pos_x, const std::string &path_neg_x,
                            const std::string &path_pos_y, const std::string &path_neg_y,
                            const std::string &path_pos_z, const std::string &path_neg_z)
    {
        MiniMat img_pos_x = loadImage(path_pos_x);
        MiniMat img_neg_x = loadImage(path_neg_x);
        MiniMat img_pos_y = loadImage(path_pos_y);
        MiniMat img_neg_y = loadImage(path_neg_y);
        MiniMat img_pos_z = loadImage(path_pos_z);
        MiniMat img_neg_z = loadImage(path_neg_z);
        
        int sz = img_pos_x.cols;
        
        CubeMap ret(sz, sz,
                    img_pos_x.data, img_neg_x.data,
                    img_pos_y.data, img_neg_y.data,
                    img_pos_z.data, img_neg_z.data);
        
        free(img_pos_x.data);
        free(img_neg_x.data);
        free(img_pos_y.data);
        free(img_neg_y.data);
        free(img_pos_z.data);
        free(img_neg_z.data);
        
        return ret;
    }
    
    CubeMap create_cube_map(const std::string &path, CubeMap::CubeMapType type)
{
    MiniMat img = loadImage(path), tmp(0, 0, 0);
    
    int sz = img.cols / 3;
    
    const uint8_t *data_pos_x = NULL, *data_neg_x = NULL, *data_pos_y = NULL, *data_neg_y = NULL,
    *data_pos_z = NULL, *data_neg_z = NULL;
    
    uint8_t data[6][sz * sz];
    
    switch (type)
    {
        case CubeMap::V_CROSS:
            sz = img.cols / 3;
            img.roi = Area<uint32_t>(2 * sz, sz, 3 * sz, 2 * sz);
            tmp = MiniMat(data[0], sz, sz, img.bytes_per_pixel);
            copyMat(img, tmp);
            data_pos_x = tmp.data;
            
            img.roi = Area<uint32_t>(0, sz, sz, 2 * sz);
            tmp = MiniMat(data[1], sz, sz, img.bytes_per_pixel);
            copyMat(img, tmp);
            data_neg_x = tmp.data;
            
            img.roi = Area<uint32_t>(sz, 0, 2 * sz, sz);
            tmp = MiniMat(data[2], sz, sz, img.bytes_per_pixel);
            copyMat(img, tmp);
            data_pos_y = tmp.data;
            
            img.roi = Area<uint32_t>(sz, 2 * sz, 2 * sz, 3 * sz);
            tmp = MiniMat(data[3], sz, sz, img.bytes_per_pixel);
            copyMat(img, tmp);
            data_neg_y = tmp.data;
            
            img.roi = Area<uint32_t>(sz, 3 * sz, 2 * sz, 4 * sz);
            tmp = MiniMat(data[4], sz, sz, img.bytes_per_pixel);
            copyMat(img, tmp);
            data_pos_z = tmp.data;
            
            img.roi = Area<uint32_t>(sz, sz, 2 * sz, 2 * sz);
            tmp = MiniMat(data[5], sz, sz, img.bytes_per_pixel);
            copyMat(img, tmp);
            data_neg_z = tmp.data;
            break;
            
        case CubeMap::H_CROSS:
            
            break;
            
        default:
            break;
    }
    
//    sz = img.cols;
//    img.roi = Area<uint32_t>(2 * sz, sz, 3 * sz, 2 * sz);
//    data_pos_x = data_neg_x = data_pos_y = data_neg_y = data_pos_z = data_neg_z = img.data_start_for_roi();;
    
    CubeMap ret(sz, sz, data_pos_x, data_neg_x, data_pos_y, data_neg_y, data_pos_z, data_neg_z);
    free(img.data);
    return ret;
}
    
CubeMap::CubeMap(GLsizei texWidth, GLsizei texHeight,
                 const uint8_t *data_pos_x, const uint8_t *data_neg_x,
                 const uint8_t *data_pos_y, const uint8_t *data_neg_y,
                 const uint8_t *data_pos_z, const uint8_t *data_neg_z)
{
	//create a texture object
	glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureObject);
    
	//assign the images to positions
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data_pos_x);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data_neg_x);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data_pos_y);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data_neg_y);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data_pos_z);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data_neg_z);
    
	//set filtering modes for scaling up and down
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void CubeMap::bindMulti( int pos )
{
	glActiveTexture(GL_TEXTURE0 + pos );
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureObject);
}

void CubeMap::bind()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureObject);
}

void CubeMap::unbind()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0 );
}

void CubeMap::enableFixedMapping()
{
//	glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP );
//	glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP );
//	glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP );
//	glEnable(GL_TEXTURE_GEN_S);
//	glEnable(GL_TEXTURE_GEN_T);
//	glEnable(GL_TEXTURE_GEN_R);
	glEnable( GL_TEXTURE_CUBE_MAP );
}

void CubeMap::disableFixedMapping()
{
//	glDisable(GL_TEXTURE_GEN_S);
//	glDisable(GL_TEXTURE_GEN_T);
//	glDisable(GL_TEXTURE_GEN_R);
	glDisable( GL_TEXTURE_CUBE_MAP );
}

}}//namespace
#endif
