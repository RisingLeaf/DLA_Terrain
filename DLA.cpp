#include "DLA.h"

#include <functional>
#include <glm/fwd.hpp>
#include <random>
#include <algorithm>


#include <GL/glew.h>



namespace {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist4(0, 3);
  std::uniform_int_distribution<std::mt19937::result_type> distI(1, 10);

  std::uniform_int_distribution<std::mt19937::result_type> dist;

  const std::vector<glm::ivec2> offsets = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
};



DLA::DLA(int size)
: size(size)
{
  // Fill the grid... with nothing.
  for(int x = 0; x < size; x++)
  {
    grid.emplace_back();
    for(int y = 0; y < size; y++)
      grid[x].emplace_back(nullptr);
  }

  // Place the central pixel that will be used to grow the dla.
  mainPixel.position = {size / 2, size / 2};
  pixels.emplace_back(&mainPixel);
  grid[size / 2][size / 2] = &mainPixel;

  // Fill the image... also with nothing.
  for(int x = 0; x < size; x++)
  {
    for(int y = 0; y < size; y++)
    {
      image.emplace_back(0.);
    }
  }
}



void DLA::Upscale()
{
  grid.clear();

  size *= SCALE_FACTOR;

  for(int x = 0; x < size; x++)
  {
    grid.emplace_back();
    for(int y = 0; y < size; y++)
      grid[x].emplace_back(nullptr);
  }

  // Recursive function to make a "sharp" upscale of the grid.
  std::function<void(Pixel*)> CreateConnections = [&](Pixel *pixel) -> void
  {
    // The connectors will be the new children.
    auto children = pixel->children;
    pixel->children.clear();

    for(Pixel *child : children)
    {
      // The connector between parent and child.
      pixels.emplace_back(new Pixel());
      pixels.back()->position = pixel->position * SCALE_FACTOR + (child->position - pixel->position);


      // Do some random jiggling to avoid straight looks.
      glm::ivec2 ortho = child->position - pixel->position;
      ortho = glm::ivec2(ortho.y, ortho.x);

      if(ortho.x)
        ortho.x = ortho.x / std::abs(ortho.x);
      if(ortho.y)
        ortho.y = ortho.y / std::abs(ortho.y);

      int ran = distI(rng);
      if(ran >= 9)
        pixels.back()->position += ortho;
      else if(ran >= 7)
        pixels.back()->position -= ortho;

      
      // Finish up linking.
      pixels.back()->parent = pixel;
      child->parent = pixels.back();

      pixel->children.emplace_back(pixels.back());
      pixels.back()->children.emplace_back(child);

      CreateConnections(child);
    }

    pixel->position *= SCALE_FACTOR;
  };

  CreateConnections(&mainPixel);

  // Place the pixels on the grid again.
  for(Pixel *pixel : pixels)
  {
    grid[pixel->position.x][pixel->position.y] = pixel;
  }
}



void DLA::UpscaleTexture()
{
  // First get the height of each pixel.
  CalculateLevels();

  imageSize = size;

  // Place the current grid onto the image.
  for(int x = 0; x < imageSize; x++)
  {
    for(int y = 0; y < imageSize; y++)
    {
      float v = grid[x][y] ? 1. - 1. / (1. + 0.5 * grid[x][y]->levels) : 0.;

      int index = x * size + y;
      float fv = 1. - 1. / (1. + (1. / step) * v + 1.25 * image[index]);

      image[index + 0] = fv;
    }
  }
  ++step;
  
  // Upscale the current image.
  std::vector<float> newImage;
  for(int x = 0; x < imageSize * SCALE_FACTOR; x++)
  {
    for(int y = 0; y < imageSize * SCALE_FACTOR; y++)
    {
      constexpr static double multiplier = 1. / static_cast<double>(SCALE_FACTOR);
      int mx = std::floor(x * multiplier) * imageSize;
      int bx = x > 0 ? std::floor((x - 1) * multiplier) * imageSize : mx;

      int my = std::floor(y * multiplier);
      int by = x > 0 ? std::floor((y - 1) * multiplier) : my;
      
      float v = 0.25 * image[bx + by] + 0.25 * image[bx + my] + 0.25 * image[mx + by] + 0.25 * image[mx + my];

      newImage.emplace_back(v);
    }
  }
  image.swap(newImage);

  imageSize = SCALE_FACTOR * size;

  // Now the image gets blurred using a convolution aproximation of gaussian blur.
  newImage.clear();
  for(int x = 0; x < imageSize; x++)
  {
    for(int y = 0; y < imageSize; y++)
    {
      int mx = x * imageSize;
      int bx = x > 0 ? (x - 1) * imageSize : mx;
      int ax = x < imageSize - 1 ? (x + 1) * imageSize : mx;

      int my = y;
      int by = y > 0 ? y - 1 : my;
      int ay = y < imageSize - 1 ? y + 1 : my;
      
      float bxby = image[bx + by]; float bxmy = image[bx + my]; float bxay = image[bx + ay];
      float mxby = image[mx + by]; float mxmy = image[mx + my]; float mxay = image[mx + ay];
      float axby = image[ax + by]; float axmy = image[ax + my]; float axay = image[ax + ay];

      float minWeight = 4. * (distI(rng) / 10.);
      float midWeight = 8. * (distI(rng) / 10.);
      float maxWeight = 16. * (distI(rng) / 10.);
      
      float v = (1. / (4. * minWeight + 4. * midWeight + maxWeight)) * (
        minWeight * bxby + midWeight * bxmy + minWeight * bxay +
        midWeight * mxby + maxWeight * mxmy + midWeight * mxay +
        minWeight * axby + midWeight * axmy + minWeight * axay
      );

      newImage.emplace_back(v);
    }
  }
  image.swap(newImage);
}



void DLA::CalculateLevels()
{
  for(Pixel *pixel : pixels)
  {
    if(pixel->children.empty())
    {
      int c = 1;
      pixel->levels = c;

      Pixel *parent = pixel->parent;
      while(parent)
      {
        c += 1;
        parent->levels = c;
        parent = parent->parent;
      }
    }
  }
}



void DLA::AddPixels(int count)
{
  dist = std::uniform_int_distribution<std::mt19937::result_type>(0, size - 1);

  for(int i = 0; i < count; i++)
  {
    // Find an empty position on the grid.
    glm::ivec2 pos = glm::ivec2(dist(rng), dist(rng));
    while(grid[pos.x][pos.y])
      pos = glm::ivec2(dist(rng), dist(rng));

    // Move the pixel around until it finds a parent.
    while(!grid[pos.x][pos.y])
    {
      for(const auto &offset : offsets)
      {
        if(pos.x + offset.x > 0 && pos.x + offset.x < size -1 && pos.y + offset.y > 0 && pos.y + offset.y < size -1)
          if(grid[pos.x + offset.x][pos.y + offset.y])
          {
            pixels.emplace_back(new Pixel());
            pixels.back()->position = pos;
            pixels.back()->parent = grid[pos.x + offset.x][pos.y + offset.y];

            grid[pos.x + offset.x][pos.y + offset.y]->children.emplace_back(pixels.back());

            grid[pos.x][pos.y] = pixels.back();
            break;
          }
      }

      if(!grid[pos.x][pos.y])
        pos += offsets[dist4(rng)];
    
      // Correct possible overshoots over the grid.
      pos.x = std::clamp(pos.x, 0, size - 1);
      pos.y = std::clamp(pos.y, 0, size - 1);
    }
  }
}



void DLA::GenTexture()
{
  // Doing another gauss blur here.
  std::vector<float> processedData;
  for(int x = 0; x < imageSize; x++)
  {
    for(int y = 0; y < imageSize; y++)
    {
      int mx = x * imageSize;
      int bx = x > 0 ? (x - 1) * imageSize : mx;
      int ax = x < imageSize - 1 ? (x + 1) * imageSize : mx;

      int my = y;
      int by = y > 0 ? y - 1 : my;
      int ay = y < imageSize - 1 ? y + 1 : my;
      
      float bxby = image[bx + by]; float bxmy = image[bx + my]; float bxay = image[bx + ay];
      float mxby = image[mx + by]; float mxmy = image[mx + my]; float mxay = image[mx + ay];
      float axby = image[ax + by]; float axmy = image[ax + my]; float axay = image[ax + ay];

      float minWeight = 4. * (distI(rng) / 10.);
      float midWeight = 8. * (distI(rng) / 10.);
      float maxWeight = 16. * (distI(rng) / 10.);
      
      float v = (1. / (4. * minWeight + 4. * midWeight + maxWeight)) * (
        minWeight * bxby + midWeight * bxmy + minWeight * bxay +
        midWeight * mxby + maxWeight * mxmy + midWeight * mxay +
        minWeight * axby + midWeight * axmy + minWeight * axay
      );

      processedData.emplace_back(v);
    }
  }

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageSize, imageSize, 0, GL_RED, GL_FLOAT, processedData.data());

  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glBindTexture(GL_TEXTURE_2D, 0);
}
