// sobel_tiled.cl
// Optimized Sobel filter using tiled local memory (halo)
//
// OpenCL kernel source, compiled at runtime by the host program. To run:
//   gcc -O2 -o sobel_host sobel_host.c -lOpenCL -lm && ./sobel_host [width] [height] [block]
//   gcc -O2 -o sobel_all_host sobel_all_host.c -lOpenCL -lm && ./sobel_all_host

__kernel void sobel_tiled(
    __global const uchar *in,
    __global uchar *out,
    const int width,
    const int height,
    __local uchar *tile)
{
    int lx = get_local_id(0);
    int ly = get_local_id(1);
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int lnx = get_local_size(0);
    int lny = get_local_size(1);

    int tile_w = lnx + 2;
    int tile_h = lny + 2;

    // Cooperative load: fill the tile (including halo)
    for (int ty = ly; ty < tile_h; ty += lny)
    {
        for (int tx = lx; tx < tile_w; tx += lnx)
        {
            int img_x = (int)(get_group_id(0) * lnx + tx - 1);
            int img_y = (int)(get_group_id(1) * lny + ty - 1);

            // clamp to image boundaries
            if (img_x < 0) img_x = 0;
            if (img_x >= width) img_x = width - 1;
            if (img_y < 0) img_y = 0;
            if (img_y >= height) img_y = height - 1;

            tile[ty * tile_w + tx] = in[img_y * width + img_x];
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (gx >= width || gy >= height) return;

    const int Gx[9] = { -1,0,1, -2,0,2, -1,0,1 };
    const int Gy[9] = { 1,2,1, 0,0,0, -1,-2,-1 };

    int cx = lx + 1, cy = ly + 1;
    int sumX = 0, sumY = 0, idx = 0;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++, idx++)
        {
            uchar p = tile[(cy + j) * tile_w + (cx + i)];
            sumX += Gx[idx] * (int)p;
            sumY += Gy[idx] * (int)p;
        }

    int mag = (int)hypot((float)sumX, (float)sumY);
    if (mag > 255) mag = 255;
    out[gy * width + gx] = (uchar)mag;
}

