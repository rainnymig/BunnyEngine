#include <FastNoise/FastNoise.h>
#include <stb_image_write.h>

#include <array>
#include <vector>
#include <algorithm>

template <typename OutT, float IMin, float IMax, float OMin, float OMax>
constexpr OutT remap(float input)
{
    static_assert(IMin != IMax);
    static_assert(OMin != OMax);

    return (input - IMin) / (IMax - IMin) * (OMax - OMin) + OMin;
}

#define remapImage remap<uint8_t, -1.0f, 1.0f, 0.0f, 255.0f>

int main()
{
    auto mainNoise = FastNoise::NewFromEncodedNodeTree(
        "FwAAAAAApHC9PwAAAAAAAIA/GQAZABkADQAFAAAAAAAAQCkAAM3MzD4AAAAAPwEbABcAAAAAAAAAgD8AAIA/"
        "AACAvwsAAQAAAAAAAAABAAAAAAAAAAAAAIA/AClcjz4BGwATABSuB0D//wMAAI/C9T0BGwATAHsUjkD//wMAAArXoz0=");
    auto detailNoise = FastNoise::NewFromEncodedNodeTree(
        "DQAEAAAAAAAAQBcAAAAAAAAAgD8AAIA/AACAvwsAAQAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAD8AAADAPw==");
    auto weatherNoise = FastNoise::NewFromEncodedNodeTree("DQACAAAAKVzPPykAAAAAAD8AAAAAAA==");

    //  main cloud noise
    //  generate 3D grid and save as 2D png
    //  every row is a layer of 2D image
    {
        constexpr unsigned int noiseDimension3D = 128;
        std::vector<float> noiseOutput(noiseDimension3D * noiseDimension3D * noiseDimension3D);
        std::vector<uint8_t> imageData(noiseDimension3D * noiseDimension3D * noiseDimension3D);
        mainNoise->GenUniformGrid3D(
            noiseOutput.data(), 0, 0, 0, noiseDimension3D, noiseDimension3D, noiseDimension3D, 0.025, 114514);
        std::transform(noiseOutput.begin(), noiseOutput.end(), imageData.begin(),
            [](const float noise) { return remapImage(noise); });
        stbi_write_png("main_cloud_noise.png", noiseDimension3D * noiseDimension3D, noiseDimension3D, 1,
            imageData.data(), sizeof(uint8_t) * noiseDimension3D * noiseDimension3D);
    }

    //  detail cloud noise
    {
        constexpr unsigned int noiseDimension3D = 32;
        std::vector<float> noiseOutput(noiseDimension3D * noiseDimension3D * noiseDimension3D);
        std::vector<uint8_t> imageData(noiseDimension3D * noiseDimension3D * noiseDimension3D);
        detailNoise->GenUniformGrid3D(
            noiseOutput.data(), 0, 0, 0, noiseDimension3D, noiseDimension3D, noiseDimension3D, 0.05, 394056);
        std::transform(noiseOutput.begin(), noiseOutput.end(), imageData.begin(),
            [](const float noise) { return remapImage(noise); });
        stbi_write_png("detail_cloud_noise.png", noiseDimension3D * noiseDimension3D, noiseDimension3D, 1,
            imageData.data(), sizeof(uint8_t) * noiseDimension3D * noiseDimension3D);
    }

    //  weather
    {
        constexpr unsigned int noiseDimension = 256;
        std::vector<float> noiseOutput(noiseDimension * noiseDimension);
        std::vector<uint8_t> imageData(noiseDimension * noiseDimension);
        weatherNoise->GenUniformGrid2D(noiseOutput.data(), 0, 0, noiseDimension, noiseDimension, 0.002f, 8451);
        std::transform(noiseOutput.begin(), noiseOutput.end(), imageData.begin(),
            [](const float noise) { return remapImage(noise); });

        stbi_write_png(
            "weather.png", noiseDimension, noiseDimension, 1, imageData.data(), sizeof(uint8_t) * noiseDimension);
    }
}
