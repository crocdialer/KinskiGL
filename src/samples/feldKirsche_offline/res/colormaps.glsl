///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Colormaps.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Grayscale colormap. Maps value directly to gray values.
 *
 * \param value the <b>scaled</b> value
 *
 * \return color.
 */
vec4 grayscale( in vec3 value )
{
    return vec4( value, 1.0 );
}

/**
 * Maps red and blue to the positive and negative part of the interval linearly. It handles the zero-point according to specified min and scale
 * values.
 *
 * \note it clips on the mid-point of the scala or 0 if minV < 0
 *
 * \param valueDescaled <b>descaled</b> value
 * \param minV min value
 * \param scaleV scaling factor
 *
 * \return pos2neg colormap
 */
vec4 negative2positive( in float valueDescaled, in float minV, in float scaleV )
{
    const vec3 zeroColor = vec3( 1.0, 1.0, 1.0 );
    const vec3 negColor = vec3( 1.0, 1.0, 0.0 );
    const vec3 posColor= vec3( 0.0, 1.0, 1.0 );

    // the descaled value can be in interval [minV,minV+Scale]. But as we want linear scaling where the pos and neg colors are scaled linearly
    // agains each other, and we want to handle real negative values correctly. For only positive values, use their interval mid-point.
    float isNegative = ( -1.0 * clamp( sign( minV ), -1.0, 0.0 ) ); // this is 1.0 if minV is smaller than zero
    float mid = ( 1.0 - isNegative ) * 0.5 * scaleV;    // if negative, the mid point always is 0.0
    // the width of the interval is its original width/2 if there are no negative values in the dataset
    float width = ( isNegative * max( abs( minV ), abs( minV + scaleV ) ) ) + ( ( 1.0 - isNegative ) * mid );

    // pos-neg mix factor
    float share = ( valueDescaled - mid ) / width;

    // use neg color for shares < 0.0 and pos color for the others
    return vec4( zeroColor - ( abs( clamp( share, -1.0 , 0.0 ) * negColor ) + ( clamp( share, 0.0 , 1.0 ) * posColor ) ),
                 clipIfValue( valueDescaled, mid ) ); // clip near mid-point
}

/**
 * Vector colormap. This basically is a grayscale colormap for each channel. It additionally clips according to vector length.
 *
 * \param valueDescaled the <b>descaled</b> vector data
 * \param minV minimum value
 * \param scaleV scaling value
 *
 * \return colormap.
 */
vec4 vector( in vec3 valueDescaled, in float minV, in float scaleV )
{
    // similar to negative2positive, we need the width of the interval.
    float m = max( abs( minV ), abs( minV + scaleV ) );
    return vec4( abs( valueDescaled / m ), 1.0 );
}

/**
 * Hot Iron colormap. It fades between red and yellowish colors.
 *
 * \param value the scaled value
 *
 * \return color.
 */
vec4 hotIron( in float value )
{
    vec4 color8  = vec4( 255.0 / 255.0, 255.0 / 255.0, 204.0 / 255.0, 1.0 );
    vec4 color7  = vec4( 255.0 / 255.0, 237.0 / 255.0, 160.0 / 255.0, 1.0 );
    vec4 color6  = vec4( 254.0 / 255.0, 217.0 / 255.0, 118.0 / 255.0, 1.0 );
    vec4 color5  = vec4( 254.0 / 255.0, 178.0 / 255.0,  76.0 / 255.0, 1.0 );
    vec4 color4  = vec4( 253.0 / 255.0, 141.0 / 255.0,  60.0 / 255.0, 1.0 );
    vec4 color3  = vec4( 252.0 / 255.0,  78.0 / 255.0,  42.0 / 255.0, 1.0 );
    vec4 color2  = vec4( 227.0 / 255.0,  26.0 / 255.0,  28.0 / 255.0, 1.0 );
    vec4 color1  = vec4( 189.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    vec4 color0  = vec4( 128.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );

    float colorValue = value * 8.0;
    int sel = int( floor( colorValue ) );

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
        colorValue -= float( sel );

        if( sel < 1 )
        {
            return ( color1 * colorValue + color0 * ( 1.0 - colorValue ) );
        }
        else if( sel < 2 )
        {
            return ( color2 * colorValue + color1 * ( 1.0 - colorValue ) );
        }
        else if( sel < 3 )
        {
            return ( color3 * colorValue + color2 * ( 1.0 - colorValue ) );
        }
        else if( sel < 4 )
        {
            return ( color4 * colorValue + color3 * ( 1.0 - colorValue ) );
        }
        else if( sel < 5 )
        {
            return ( color5 * colorValue + color4 * ( 1.0 - colorValue ) );
        }
        else if( sel < 6 )
        {
            return ( color6 * colorValue + color5 * ( 1.0 - colorValue ) );
        }
        else if( sel < 7 )
        {
            return ( color7 * colorValue + color6 * ( 1.0 - colorValue ) );
        }
        else if( sel < 8 )
        {
            return ( color8 * colorValue + color7 * ( 1.0 - colorValue ) );
        }
        else
        {
            return color0;
        }
    }
}

/**
 * Rainbow colormap. Fading through the rainbow colors.
 *
 * \param value the value in [0,1]
 *
 * \return color
 */
vec4 rainbow( in float value )
{
    float i = floor( 6.0 * value );
    float f = 6.0 * value - i;
    float q = 1.0 - f;

    int iq = int( mod( i, 6.0 ) );

    if( ( iq == 0 ) || ( iq == 6 ) )
    {
        return vec4( 1.0, f, 0.0, 1.0 );
    }
    else if( iq == 1 )
    {
        return vec4( q, 1.0, 0.0, 1.0 );
    }
    else if( iq == 2 )
    {
        return vec4( 0.0, 1.0, f, 1.0 );
    }
    else if( iq == 3 )
    {
        return vec4( 0.0, q, 1.0, 1.0 );
    }
    else if( iq == 4 )
    {
        return vec4( f, 0.0, 1.0, 1.0 );
    }
    else // iq == 5
    {
        return vec4( 1.0, 0.0, q, 1.0 );
    }
}

/**
 * Colormap fading between green, blue and purple.
 *
 * \param value the scaled value in [0,1]
 *
 * \return the color
 */
vec4 blueGreenPurple( in float value )
{
    value *= 5.0;
    vec4 color;
    if( value < 0.0 )
    {
        color = vec4( 0.0, 0.0, 0.0, 1.0 );
    }
    else if( value < 1.0 )
    {
        color = vec4( 0.0, value, 1.0, 1.0 );
    }
    else if( value < 2.0 )
    {
        color = vec4( 0.0, 1.0, 2.0 - value, 1.0 );
    }
    else if( value < 3.0 )
    {
        color =  vec4( value - 2.0, 1.0, 0.0, 1.0 );
    }
    else if( value < 4.0 )
    {
        color = vec4( 1.0, 4.0 - value, 0.0, 1.0 );
    }
    else if( value <= 5.0 )
    {
        color = vec4( 1.0, 0.0, value - 4.0, 1.0 );
    }
    else
    {
        color =  vec4( 1.0, 0.0, 1.0, 1.0 );
    }

    return color;
}

/**
 * Colormap especially suitable for mask data. It tries to find distinct colors for neighbouring values.
 *
 * \param value the value in [0,1]
 *
 * \return the volor
 */
vec4 atlas( in float value )
{
    float val = floor( value * 255.0 );
    float r = 0.0;
    float g = 0.0;
    float b = 0.0;
    float mult = 1.0;

    if( val == 0.0 )
    {
        return vec4( vec3( 0.0 ), 1.0 );
    }

    if( isBitSet( val, 0.0 ) )
    {
        b = 1.0;
    }
    if( isBitSet( val, 1.0 ) )
    {
        g = 1.0;
    }
    if( isBitSet( val, 2.0 ) )
    {
        r = 1.0;
    }
    if( isBitSet( val, 3.0 ) )
    {
        mult -= 0.15;
        if( r < 1.0 && g < 1.0 && b < 1.0 )
        {
            r = 1.0;
            g = 1.0;
        }
    }
    if( isBitSet( val, 4.0 ) )
    {
        mult -= 0.15;
        if( r < 1.0 && g < 1.0 && b < 1.0 )
        {
            b = 1.0;
            g = 1.0;
        }
    }
    if( isBitSet( val, 5.0 ) )
    {
        mult -= 0.15;
        if( r < 1.0 && g < 1.0 && b < 1.0 )
        {
            r = 1.0;
            b = 1.0;
        }
    }
    if( isBitSet( val, 6.0 ) )
    {
        mult -= 0.15;
        if( r < 1.0 && g < 1.0 && b < 1.0 )
        {
            g = 1.0;
        }
    }
    if( isBitSet( val, 7.0 ) )
    {
        mult -= 0.15;
        if( r < 1.0 && g < 1.0 && b < 1.0 )
        {
            r = 1.0;
        }
    }

    r *= mult;
    g *= mult;
    b *= mult;

    clamp( r, 0.0, 1.0 );
    clamp( g, 0.0, 1.0 );
    clamp( b, 0.0, 1.0 );

    return vec4( r, g, b, 1.0 );
}

/**
 * This method applies a colormap to the specified value an mixes it with the specified color. It uses the proper colormap and works on scaled
 * values.
 *
 * \return this color gets mixed using alpha value with the new colormap color
 * \param value the value to map, <b>scaled</b>
 * \param minV the minimum of the original value
 * \param scaleV the scaler used to downscale the original value to [0-1]
 * \param thresholdV a threshold in original space (you need to downscale it to [0-1] if you want to use it to scaled values.
 * \param thresholdEnabled a flag denoting whether threshold-based clipping should be done or not
 * \param alpha the alpha blending value
 * \param colormap the colormap index to use
 */
vec4 colormap( in vec4 value, float minV, float scaleV, float thresholdV, bool thresholdEnabled, float alpha, int colormap, bool active )
{
    // descale value
    vec3 valueDescaled = vec3( minV ) + ( value.rgb * scaleV );
    float isNotBorder = float( value.a >= 0.75 );

    // this is the final color returned by the colormapping algorithm. This is the correct value for the gray colormap
    vec4 cmapped = grayscale( value.rgb );
    float clip = clipZero( valueDescaled.r, minV );

    // negative to positive shading in red-blue
    if( colormap == 1 )
    {
        cmapped = rainbow( value.r );
    }
    else if( colormap == 2 )
    {
        cmapped = hotIron( value.r );
    }
    else if( colormap == 3 )
    {
        cmapped = negative2positive( valueDescaled.r, minV, scaleV );
    }
    else if( colormap == 4 )
    {
        cmapped = atlas( value.r );
    }
    else if( colormap == 5 )
    {
        cmapped = blueGreenPurple( value.r );
    }
    else if( colormap == 6 )
    {
        cmapped = vector( valueDescaled, minV, scaleV );
        clip = clipZero( valueDescaled );   // vectors get clipped by their length
    }

    // build final color
    return vec4( cmapped.rgb, cmapped.a *           // did the colormap use a alpha value?
                              isNotBorder *         // is this a border pixel?
                              alpha *               // did the user specified an alpha?
                              clip *                // value clip?
                              clipThreshold( valueDescaled, colormap, thresholdV, thresholdEnabled ) * // clip due to threshold?
                              float( active ) );    // is it active?
}
