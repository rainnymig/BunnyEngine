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
    auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
    auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();
    fnFractal->SetSource(fnSimplex);
    fnFractal->SetOctaveCount(5);
    fnFractal->SetLacunarity(2);
    fnFractal->SetWeightedStrength(0.5);

    constexpr unsigned int noiseDimension = 1024;

    std::vector<float> noiseOutput(noiseDimension * noiseDimension);

    fnFractal->GenUniformGrid2D(noiseOutput.data(), 0, 0, noiseDimension, noiseDimension, 0.02f, 1994);

    std::vector<uint8_t> imageData(noiseDimension * noiseDimension);
    std::transform(
        noiseOutput.begin(), noiseOutput.end(), imageData.begin(), [](const float noise) { return remapImage(noise); });

    stbi_write_png(
        "test_simplex.png", noiseDimension, noiseDimension, 1, imageData.data(), sizeof(uint8_t) * noiseDimension);
}
