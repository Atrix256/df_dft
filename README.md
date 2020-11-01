# df_dft

Simple command line to dft/idft images.

Usage:
`DFT <dft|idft> <source file> <dest file>`

when you dft an image, the source file is an image file.  It turns it to greyscale, and then generates the following files:
* .raw.png - this is the greyscale image given to dft.
* .mag.png - this shows the magnitude data of the dft.  it is put through a log function and normalized to have the largest value of 1.0
* .phase.png - this shows the phase information. -pi to +pi is mapped to 0 to 1.
* .dft - this is a binary file that has uint32 width and height, then w*h*2 doubles for the dft data.
* .csv - this has the raw dft data as well as the magnitude and phase data, in a csv format.

When you idft an image, the source file is a .idft file made via the dft command. it will then do an inverse dft and spit out the results.

This utility uses:
* simple_fft to do dft and idft operations: https://github.com/d1vanov/Simple-FFT
* stb to read and write images: https://github.com/nothings/stb

# Example: LokiAlan

![](assets/lokialan.jpg?raw=true)

```DFT dft assets/LokiAlan.jpg out/LokiAlan```

Frequency Magnitudes:

![](out/LokiAlan.mag.png?raw=true)

Frequency Phases:

![](out/LokiAlan.phase.png?raw=true)

```DFT idft out/LokiAlan.dft out/LokiAlanRT```

![](out/LokiAlanRT.png?raw=true)

# Example: BlueNoise

![](assets/BlueNoise.png?raw=true)

```DFT dft assets/BlueNoise.png out/BlueNoise```

Frequency Magnitudes:

![](out/BlueNoise.mag.png?raw=true)

Frequency Phases:

![](out/BlueNoise.phase.png?raw=true)

```DFT idft out/BlueNoise.dft out/BlueNoiseRT```

![](out/BlueNoiseRT.png?raw=true)
