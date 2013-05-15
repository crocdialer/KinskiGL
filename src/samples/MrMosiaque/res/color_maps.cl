/**
 * Hot Iron colormap. It fades between red and yellowish colors.
 *
 * \param value the scaled value
 *
 * \return color.
 */
float4 hotIron(float value)
{
    float4 color8  = (float4)( 255.0 / 255.0, 255.0 / 255.0, 204.0 / 255.0, 1.0 );
    float4 color7  = (float4)( 255.0 / 255.0, 237.0 / 255.0, 160.0 / 255.0, 1.0 );
    float4 color6  = (float4)( 254.0 / 255.0, 217.0 / 255.0, 118.0 / 255.0, 1.0 );
    float4 color5  = (float4)( 254.0 / 255.0, 178.0 / 255.0,  76.0 / 255.0, 1.0 );
    float4 color4  = (float4)( 253.0 / 255.0, 141.0 / 255.0,  60.0 / 255.0, 1.0 );
    float4 color3  = (float4)( 252.0 / 255.0,  78.0 / 255.0,  42.0 / 255.0, 1.0 );
    float4 color2  = (float4)( 227.0 / 255.0,  26.0 / 255.0,  28.0 / 255.0, 1.0 );
    float4 color1  = (float4)( 189.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    float4 color0  = (float4)( 128.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    
    float colorValue = value * 8.0;
    int sel = (int)( floor( colorValue ) );
    
    if( sel >= 8 )
    {
        return color0;
    }
    else if( sel < 0 )
    {
        return color0;
    }
    else
    {
        colorValue -= (float)( sel );
        
        if( sel < 1 )
        {
            return ( color1 * colorValue + color0 * ( 1.0f - colorValue ) );
        }
        else if( sel < 2 )
        {
            return ( color2 * colorValue + color1 * ( 1.0f - colorValue ) );
        }
        else if( sel < 3 )
        {
            return ( color3 * colorValue + color2 * ( 1.0f - colorValue ) );
        }
        else if( sel < 4 )
        {
            return ( color4 * colorValue + color3 * ( 1.0f - colorValue ) );
        }
        else if( sel < 5 )
        {
            return ( color5 * colorValue + color4 * ( 1.0f - colorValue ) );
        }
        else if( sel < 6 )
        {
            return ( color6 * colorValue + color5 * ( 1.0f - colorValue ) );
        }
        else if( sel < 7 )
        {
            return ( color7 * colorValue + color6 * ( 1.0f - colorValue ) );
        }
        else if( sel < 8 )
        {
            return ( color8 * colorValue + color7 * ( 1.0f - colorValue ) );
        }
        else
        {
            return color0;
        }
    }
}

float4 gray(float4 color)
{
    float y_val = dot(color.xyz, (float3)(0.299, 0.587, 0.114));
    return (float4)(y_val, y_val, y_val, color.w);
}

inline float4 jet(float val)
{
    return (float4)(min(4.0 * val - 1.5, -4.0 * val + 4.5),
                    min(4.0 * val - 0.5, -4.0 * val + 3.5),
                    min(4.0 * val + 0.5, -4.0 * val + 2.5),
                    1.0);
}