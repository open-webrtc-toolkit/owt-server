/* <COPYRIGHT_TAG> */

#define CORRECTCOLOR(x) ((x) < 0.000001f ? ((uchar)(x)) : ((ushort)(x) > 255 ? 255 : (uchar)(x)))

__kernel void yuv2rgb(__read_only image2d_t y_plane, __read_only image2d_t uv_plane, __global uchar* rgb_img)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int width = get_global_size(0);
    int2 coord_src = (int2)(x,y);

    const sampler_t smp = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    float4 y_pixel = read_imagef(y_plane, smp, coord_src);
    float4 uv_pixel = read_imagef(uv_plane, smp, coord_src/2);

    float yc = y_pixel.x * 255.0f;
    float uc = uv_pixel.x * 255.0f;
    float vc = uv_pixel.y * 255.0f;

    float r = yc + 1.4075f * (vc-128.0f);
    float g = yc - 0.3455f * (uc-128.0f) - 0.7169f * (vc-128.0f);
    float b = yc + 1.779f * (uc-128.0f);

    uchar rgb_r = CORRECTCOLOR(r);
    uchar rgb_g = CORRECTCOLOR(g);
    uchar rgb_b = CORRECTCOLOR(b);

    int idx = (width * y + x) * 3;
    rgb_img[idx] = rgb_b;
    rgb_img[idx+1] = rgb_g;
    rgb_img[idx+2] = rgb_r;
}

__kernel void yuv2gray(__read_only image2d_t y_plane, __read_only image2d_t uv_plane, __global uchar* gray_img)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int width = get_global_size(0);
    int2 coord_src = (int2)(x,y);

    const sampler_t smp = CLK_FILTER_NEAREST;
    float4 y_pixel = read_imagef(y_plane, smp, coord_src);

    float yc = y_pixel.x * 255.0f;
    uchar gray_y = CORRECTCOLOR(yc);
    gray_img[width * y + x] = gray_y;
}

__kernel void rgba2hsv(__read_only image2d_t bgra_img, __write_only image2d_t hsv_img)
{
	//TODO: implement this kernel in later version
}
