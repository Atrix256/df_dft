#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "simple_fft/fft_settings.h"
#include "simple_fft/fft.h"

#include <vector>

// if true, will put magntiude through a log function before making the magnitude image
#define MAGNITUDE_LOG() true

// If true, will zero out the DC magnitude from the magnitude image before showing it, since DC is usually very large.
#define MAGNITUDE_ZERODC() false

static const float c_pi = 3.14159265359f;

inline bool IsPowerOf2(size_t n)
{
    return (n & (n - 1)) == 0;
}

template <typename T>
T Clamp(T value, T min, T max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

struct ComplexImage2D
{
    ComplexImage2D(size_t w = 0, size_t h = 0)
    {
        Resize(w, h);
    }

    size_t m_width;
    size_t m_height;
    std::vector<complex_type> pixels;

    void Resize(size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        pixels.resize(w * h, real_type(0.0f));
    }

    complex_type& operator()(size_t x, size_t y)
    {
        return pixels[y * m_width + x];
    }

    const complex_type& operator()(size_t x, size_t y) const
    {
        return pixels[y * m_width + x];
    }

    void IndexToCoordinates(size_t index, size_t& x, size_t& y) const
    {
        y = index / m_width;
        x = index % m_width;
    }
};

void DFT(const char* srcFileName, const char* destFileName)
{
    char buffer[4096];

    // load image and make a ComplexImage2D
    ComplexImage2D complexImageIn;
    {
        // load the image
        int w, h, comp;
        stbi_uc* pixels = stbi_load(srcFileName, &w, &h, &comp, 1);
        if (!pixels)
        {
            printf(__FUNCTION__ "(): Could not load image %s", srcFileName);
            return;
        }

        // convert to complex pixels
        complexImageIn.Resize(w, h);
        for (size_t index = 0; index < complexImageIn.pixels.size(); ++index)
            complexImageIn.pixels[index] = float(pixels[index]) / 255.0f;

        // write out the source image
        strcpy(buffer, destFileName);
        strcat(buffer, ".raw.png");
        stbi_write_png(buffer, w, h, 1, pixels, w);

        // free the pixels
        stbi_image_free(pixels);

        // verify the image is a power of 2
        if (!IsPowerOf2(w) || !IsPowerOf2(h))
        {
            printf(__FUNCTION__ "(): image is %ix%i but width and height need to be a power of 2", w, h);
            return;
        }
    }

    // DFT the image
    ComplexImage2D complexImageOut;
    {
        const char* error = nullptr;
        complexImageOut.Resize(complexImageIn.m_width, complexImageIn.m_height);
        simple_fft::FFT(complexImageIn, complexImageOut, complexImageIn.m_width, complexImageIn.m_height, error);
    }

    // write the magnitude and phase images - with dc in the middle, instead of at (0,0)
    {
        std::vector<float> mag(complexImageOut.pixels.size());
        std::vector<float> phase(complexImageOut.pixels.size());

        float maxMag = 0.0f;
        for (size_t index = 0; index < complexImageOut.pixels.size(); ++index)
        {
            size_t px, py;
            complexImageOut.IndexToCoordinates(index, px, py);
            px = (px + complexImageOut.m_width / 2) % complexImageOut.m_width;
            py = (py + complexImageOut.m_height / 2) % complexImageOut.m_height;
            const complex_type& c = complexImageOut(px, py);

            #if MAGNITUDE_LOG()
            mag[index] = float(log(1.0f + float(sqrt(c.real() * c.real() + c.imag() * c.imag()))));
            #else
            mag[index] = float(sqrt(c.real() * c.real() + c.imag() * c.imag()));
            #endif

            phase[index] = float(atan2(c.imag(), c.real()));

            #if MAGNITUDE_ZERODC()
            // removing DC (0hz)
            if (px == 0 && py == 0)
                mag[index] = 0.0f;
            #endif

            maxMag = std::max(maxMag, mag[index]);
        }

        // write magnitude image
        std::vector<uint8_t> pixels(complexImageOut.pixels.size());
        for (size_t index = 0; index < complexImageOut.pixels.size(); ++index)
            pixels[index] = (uint8_t)Clamp(mag[index] / maxMag * 256.0f, 0.0f, 255.0f);
        strcpy(buffer, destFileName);
        strcat(buffer, ".mag.png");
        stbi_write_png(buffer, (int)complexImageOut.m_width, (int)complexImageOut.m_height, 1, pixels.data(), (int)complexImageOut.m_width);

        // write phase image
        for (size_t index = 0; index < complexImageOut.pixels.size(); ++index)
        {
            float value = (phase[index] + c_pi) / (2.0f * c_pi);
            pixels[index] = (uint8_t)Clamp(value * 256.0f, 0.0f, 255.0f);
        }
        strcpy(buffer, destFileName);
        strcat(buffer, ".phase.png");
        stbi_write_png(buffer, (int)complexImageOut.m_width, (int)complexImageOut.m_height, 1, pixels.data(), (int)complexImageOut.m_width);
    }

    // write csv
    {
        FILE* file = nullptr;
        strcpy(buffer, destFileName);
        strcat(buffer, ".csv");
        fopen_s(&file, buffer, "wb");

        fprintf(file, "\"index\",\"x\",\"y\",\"dft real\",\"dft imaginary\",\"magnitude\",\"phase\"\n");

        for (size_t index = 0; index < complexImageOut.pixels.size(); ++index)
        {
            const complex_type& c = complexImageOut.pixels[index];

            float mag = float(sqrt(c.real() * c.real() + c.imag() * c.imag()));
            float phase = float(atan2(c.imag(), c.real()));

            size_t x, y;
            complexImageOut.IndexToCoordinates(index, x, y);
            fprintf(file, "\"%i\",\"%i\",\"%i\",\"%f\",\"%f\",\"%f\",\"%f\"\n",
                (int)index,
                (int)x,
                (int)y,
                c.real(),
                c.imag(),
                mag,
                phase
            );
        }

        fclose(file);
    }

    // write out the dft data
    {
        FILE* file = nullptr;
        strcpy(buffer, destFileName);
        strcat(buffer, ".dft");
        fopen_s(&file, buffer, "wb");

        uint32_t w = (uint32_t)complexImageOut.m_width;
        uint32_t h = (uint32_t)complexImageOut.m_height;
        fwrite(&w, sizeof(w), 1, file);
        fwrite(&h, sizeof(h), 1, file);

        fwrite(complexImageOut.pixels.data(), sizeof(double) * 2 * complexImageOut.pixels.size(), 1, file);

        fclose(file);
    }
}

void IDFT(const char* fileName, const char* destFileName)
{
    char buffer[4096];

    // read in the complex image
    ComplexImage2D complexImageIn;
    {
        FILE* file = nullptr;
        fopen_s(&file, fileName, "rb");
        uint32_t w, h;
        fread(&w, sizeof(w), 1, file);
        fread(&h, sizeof(h), 1, file);

        complexImageIn.Resize(w, h);

        fread(complexImageIn.pixels.data(), sizeof(double) * 2 * complexImageIn.pixels.size(), 1, file);

        fclose(file);
    }

    // IDFT the image
    ComplexImage2D complexImageOut;
    {
        const char* error = nullptr;
        complexImageOut.Resize(complexImageIn.m_width, complexImageIn.m_height);
        simple_fft::IFFT(complexImageIn, complexImageOut, complexImageIn.m_width, complexImageIn.m_height, error);
    }

    // convert to U8 and write it out
    {
        std::vector<uint8_t> pixels;
        pixels.resize(complexImageOut.pixels.size());

        for (size_t index = 0; index < complexImageOut.pixels.size(); ++index)
        {
            const complex_type& c = complexImageOut.pixels[index];
            float mag = float(sqrt(c.real() * c.real() + c.imag() * c.imag()));

            pixels[index] = (uint8_t)Clamp(mag * 256.0f, 0.0f, 255.0f);
        }

        strcpy(buffer, destFileName);
        strcat(buffer, ".png");
        stbi_write_png(buffer, (int)complexImageOut.m_width, (int)complexImageOut.m_height, 1, pixels.data(), (int)complexImageOut.m_width);
    }
}

void ShowUsage()
{
    printf(
        "Usage:\n"
        "  DFT <dft|idft> <source file> <dest file>\n\n"
    );
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        ShowUsage();
        return 0;
    }

    if (!_stricmp(argv[1], "dft"))
    {
        DFT(argv[2], argv[3]);
    }
    else if (!_stricmp(argv[1], "idft"))
    {
        IDFT(argv[2], argv[3]);
    }
    else
    {
        ShowUsage();
    }

    return 0;
}
