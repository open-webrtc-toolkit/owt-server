/* <COPYRIGHT_TAG> */

__kernel void backgroundsubtract_abs_diff(__global unsigned char*  background, __read_only image2d_t y_plane, __global unsigned char* out_img)
{
    uint result;
    unsigned char ch;

    int x = get_global_id(0);
    int y = get_global_id(1);
    int width = get_global_size(0);
    int2 coord_src = (int2)(x,y);

    const sampler_t smp = CLK_FILTER_NEAREST;
    float4 img_pixel = read_imagef(y_plane, smp, coord_src);
    int yc = (int)(img_pixel.x * 255.0f);
    //float4 background_pixel = read_imagef(background, smp, coord_src);
    //int bc = (int)(background_pixel.x * 255.0f);
    int bc = background[width * y + x];
    result = abs_diff(yc, bc);
    if(result < 30)
        ch = 0;
    else
        ch = 255;

    out_img[ width * y + x ] = ch;
}

//TODO: implement gmm background update
__kernel void backgroundsubtract_gmm(__read_only image2d_t background, __read_only image2d_t y_plane, __global unsigned char* out_img)
{
    //TODO:
}
