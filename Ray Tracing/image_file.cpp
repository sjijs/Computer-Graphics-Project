#include <iostream>
#include <fstream>

int main() {

    // Image

    int image_width = 256;
    int image_height = 256;

    // 创建输出文件流
    std::ofstream file("image_direct.ppm");
    
    // Render

    file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        for (int i = 0; i < image_width; i++) {
            auto r = double(i) / (image_width-1);
            auto g = double(j) / (image_height-1);
            auto b = 0.0;

            int ir = int(255.999 * r);
            int ig = int(255.999 * g);
            int ib = int(255.999 * b);

            file << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }
    
    file.close();
    std::cout << "PPM file generated successfully!\n";
    
    return 0;
}
