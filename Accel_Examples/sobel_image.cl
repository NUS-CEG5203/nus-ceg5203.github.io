// sobel_image.cl
// Sobel edge detection using image objects and samplers
//
// OpenCL kernel source, compiled at runtime by the host program. To run:
//   gcc -O2 -o sobel_all_host sobel_all_host.c -lOpenCL -lm && ./sobel_all_host

__constant sampler_t smp = CLK_NORMALIZED_COORDS_FALSE |
                           CLK_ADDRESS_CLAMP_TO_EDGE |
                           CLK_FILTER_NEAREST;

__kernel void sobel_image(read_only image2d_t srcImg,
                          write_only image2d_t dstImg)
{
    int2 coord = (int2)(get_global_id(0), get_global_id(1));

    // Fetch 3x3 neighborhood
    float4 p00 = read_imagef(srcImg, smp, coord + (int2)(-1, -1));
    float4 p01 = read_imagef(srcImg, smp, coord + (int2)(0, -1));
    float4 p02 = read_imagef(srcImg, smp, coord + (int2)(1, -1));
    float4 p10 = read_imagef(srcImg, smp, coord + (int2)(-1, 0));
    float4 p11 = read_imagef(srcImg, smp, coord + (int2)(0, 0));
    float4 p12 = read_imagef(srcImg, smp, coord + (int2)(1, 0));
    float4 p20 = read_imagef(srcImg, smp, coord + (int2)(-1, 1));
    float4 p21 = read_imagef(srcImg, smp, coord + (int2)(0, 1));
    float4 p22 = read_imagef(srcImg, smp, coord + (int2)(1, 1));

    // Sobel operators (applied to red channel)
    float gx = (-p00.x - 2.0f*p10.x - p20.x) + (p02.x + 2.0f*p12.x + p22.x);
    float gy = ( p00.x + 2.0f*p01.x + p02.x) - (p20.x + 2.0f*p21.x + p22.x);

    float mag = sqrt(gx*gx + gy*gy);
    if (mag > 1.0f) mag = 1.0f;

    write_imagef(dstImg, coord, (float4)(mag, mag, mag, 1.0f));
}

