# DLA_Terrain
 
This code roughly follows the instructions given in the second part of this video: https://www.youtube.com/watch?v=gsJHzBTPG0Y

DLA or Diffusion Limited Aggregation (https://en.wikipedia.org/wiki/Diffusion-limited_aggregation) is combined with image processing such as convolution (https://en.wikipedia.org/wiki/Kernel_(image_processing)) specifically an aproximation of Gaussian blur to create a heightmap that resembles a mountain range.

The code is written in c++ and only depends glm because I have not yet found the time to create my own ivec2. It also depends on glew so that it is able to create an openGL texture, this code could be removed with ease though.
Functions are documented in the header, here is an example code:
```c++
// Create a dla with 12x12 dimensions.
DLA dla(12);
// Add 12 pixels.
dla.AddPixels(12);
// Upscale the texture.
dla.UpscaleTexture();

// Upscale the pixel grid.
dla.Upscale();
// etc.
dla.AddPixels(24);
dla.UpscaleTexture();

dla.Upscale();
dla.AddPixels(9 * 24);
dla.UpscaleTexture();

dla.Upscale();
dla.AddPixels(9 * 9 * 24);
dla.UpscaleTexture();

// Generate an openGL tetxure from the image data.
dla.GenTexture();
```
