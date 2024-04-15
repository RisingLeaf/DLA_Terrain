#ifndef PTREE_DLA_H
#define PTREE_DLA_H

#include <vector>
#include <glm/glm.hpp>



// Diffusion Limited Aggregation for terrain generation.
class DLA
{
public:
  // Represents a placed pixel in the grid, also stores the pixel it connected to and all pixels that used this one as base.
  struct Pixel {
    glm::ivec2 position;
    Pixel* parent = nullptr;
    int levels = 0;
    std::vector<Pixel *> children = {};
  };

  // This can only be 2 for now.
  // TODO: update sharp upscaling process.
  constexpr static int SCALE_FACTOR = 2;

private:
  // Current size of the grid.
  int size;
  // Current size of the image(heightmap).
  int imageSize;

  // How often this DLA was upscaled already.
  int step = 1;

  // The inital pixel in the center of the grid.
  Pixel mainPixel;

  // Every pixel on the grid.
  std::vector<Pixel *> pixels;

  // The grid, used for pixel placement and "sharp" upscaling.
  std::vector<std::vector<Pixel *>> grid;

  // The image, this is where the heightmap gets assembled.
  std::vector<float> image;

  // GL id of the texture.
  unsigned texture;

public:
  DLA(int size);

  /* Example of a generation:
    DLA dla(12);
    dla.AddPixels(12);
    dla.UpscaleTexture();
    
    dla.Upscale();
    dla.AddPixels(24);
    dla.UpscaleTexture();
    
    dla.Upscale();
    dla.AddPixels(9 * 24);
    dla.UpscaleTexture();

    dla.Upscale();
    dla.AddPixels(9 * 9 * 24);
    dla.UpscaleTexture();

    dla.Upscale();
    dla.AddPixels(3 * 9 * 9 * 24);
    dla.UpscaleTexture();
    
    dla.GenTexture();

    Note that the steps UpscaleTexture(); and Upscale(); have to be used consecutively.
  */

  // Upscale the grid.
  void Upscale();
  // Upscale the image, this involves some post processing.
  void UpscaleTexture();
  // Add count pixels to the grid.
  void AddPixels(int count);
  // Generate the final heightmap, this also blurs the image a bit.
  void GenTexture();

  const std::vector<Pixel*> &Pixels() const { return pixels; }
  unsigned GetTexture() const { return texture; }

private:
  void CalculateLevels();
};

#endif //PTREE_DLA_H
