// sobel_naive.cl
// Simple Sobel kernel (each work-item reads 9 pixels from global memory)
//
// OpenCL kernel source, compiled at runtime by the host program. To run:
//   gcc -O2 -o sobel_host sobel_host.c -lOpenCL -lm && ./sobel_host [width] [height] [block]
//   gcc -O2 -o sobel_all_host sobel_all_host.c -lOpenCL -lm && ./sobel_all_host

__kernel void sobel_naive(
    __global const uchar *in,
    __global uchar *out,
    const int width,
    const int height)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x <= 0 || y <= 0 || x >= width - 1 || y >= height - 1)
    {
        out[y * width + x] = 0;
        return;
    }

    const int Gx[3][3] = { {-1,0,1}, {-2,0,2}, {-1,0,1} };
    const int Gy[3][3] = { {1,2,1}, {0,0,0}, {-1,-2,-1} };

    int sumX = 0, sumY = 0;

    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
        {
            int p = in[(y + j) * width + (x + i)];
            sumX += Gx[j + 1][i + 1] * p;
            sumY += Gy[j + 1][i + 1] * p;
        }

    int mag = (int)hypot((float)sumX, (float)sumY);
    if (mag > 255) mag = 255;
    out[y * width + x] = (uchar)mag;
}

