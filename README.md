# RGB <-> YCoCg Conversion Implementation and Tests

## MOTIVATION

While YCoCg seems to be the modern standard for Luma + Chrominance encoding I could not find an integer based encoder/decoder implementation that actually worked as expected when tested. The [original paper](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/06/Malvar_Sullivan_YCoCg-R_JVT-I014r3-2.pdf) is also not very clear on some important points which can make correct implementation difficult.

The header [ycocg.h](https://github.com/aaron-clovsky/ycocg/blob/main/include/ycocg.h) is entirely self contained and provides inline functions for color space conversion of single pixels, rows of pixels as well as entire images. Additionally, SIMD aceleration is implemented and used automatically when available.

Currently [ycocg.h](https://github.com/aaron-clovsky/ycocg/blob/main/include/ycocg.h) provides functions for:
- Conversion of RGB values to and from YCoCg
- 4:2:2 compression and decompression from/to BGRA images with SIMD acceleration (and associated utilities)

In addition to the provided header and tests, YCoCg to RGB decoder modules and testbenches are provided for Verilog and SystemVerilog.

## FUTURE DIRECTION

- Support 4:2:0 compression and decompression
- Support AVX512 and NEON
- Support more pixel formats

## CREDIT

The authors of the [original paper](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/06/Malvar_Sullivan_YCoCg-R_JVT-I014r3-2.pdf).
 
## LINUX BUILD INSTRUCTIONS

- Install a C compiler if one is not present
  (Run ```sudo apt install build-essential``` on Debian-based distros)
- Install Verilator (if running testbenches)
  (Run ```sudo apt install verilator``` on Debian-based distros)
- ```cd``` to this repo's directory
- Run ```make``` to build tests
- Run ```make testbench``` to run Verilog and SystemVerilog testbenches

## WINDOWS BUILD INSTRUCTIONS

- Install the latest Visual Studio and Visual C++
- Open a "Native Tools Command Prompt for VS" console for your preferred architecture
- ```cd``` to this repo's directory
- Run ```msbuild``` to build tests

## TESTS

```
ycocg_test <MODE> [OPTIONS]
 modes: rgb | ycocg | analyze | all | allrgb | allycocg
  rgb options: <r> <g> <b>
  ycocg options: <y> <co> <cg>
```
Quantitatively analyzes and exports the results of the ```ycocg_from_rgb()``` and ```ycocg_to_rgb()``` functions.

#### MODES & OPTIONS

```rgb <r> <g> <b>```
Converts a given 24-bit RGB value to a 24-bit YCoCg value

```ycocg <y> <co> <cg>```
Converts a given 24-bit YCoCg value to a 24-bit RGB value

```analyze```
Converts every 24-bit RGB value first to 24-bit YCoCg then back to 24-bit RGB and gathers statistics on conversion accuracy

```all```
Prints the following in 9 column CSV output:
- Columns 1-3: Every possible 24-bit RGB value
- Columns 4-6: The 24-bit YCoCg conversion of the RGB values in columns 1-3
- Columns 7-9: The 24-bit RGB conversion of the YCoCg values in columns 4-6

```allrgb```
Prints the following in 6 column CSV output:
- Columns 1-3: Every possible 24-bit RGB value
- Columns 4-6: The 24-bit YCoCg conversion of the RGB values in columns 1-3

```allycocg```
Prints the following in 6 column CSV output:
- Columns 1-3: Every possible 24-bit YCoCg value
- Columns 4-6: The 24-bit RGB conversion of the YCoCg values in columns 1-3

##

```
ycocg_simd_rand [width height repetitions]
```
Tests the performance and accuracy of SIMD accelerated functions against their scalar reference implementations using randomized data.

#### OPTIONS

* **`width`** - Width of test image
* **`height`** - Height of test image
* **`repetitions`** - Number of times to repeat the test

##

```
ycocg_simd_full <TEST> [starting block] [block count]
 tests: yuy2_from_bgrx | yuy2_to_bgra
```
Tests the results of SIMD accelerated functions against their scalar reference implementations for the entire input range. Ranges are divided into 10,000 blocks for partial and/or parallel processing, if the range does not cleanly divide by 10,000 then the first block also contains the remainder.

#### TESTS

* **`yuy2_from_bgrx`** - Tests all possible 48-bit inputs to ```ycocg_yuy2_from_bgrx()``` against ```ycocg_yuy2_from_bgrx_scalar()```
* **`yuy2_to_bgra`** -  Tests all possible 32-bit inputs to ```ycocg_yuy2_to_bgra()``` against ```ycocg_yuy2_to_bgra_scalar()```

#### OPTIONS

* **`starting block`** - First block to test.
* **`block count`** - Number of blocks to test.

## NOTES

- [ycocg.h](https://github.com/aaron-clovsky/ycocg/blob/main/include/ycocg.h) is C99+ and C++98+ compatible, however it is also C89 compatible if ```stdint.h``` is available or if the types: ```int8_t```, ```uint8_t```, ```int16_t```, ```uint32_t ``` and ```uint64_t ``` are typedef'd prior to inclusion and ```YCOCG_C89``` is #define'd.
- ```make style``` is implemented to keep code within style guidelines, it requires clang-format-21
- ```make lint``` is implemented to help check for warnings, it requires: gcc, g++, clang, clang++ and cppcheck

## MATH NOTES

- The decoding formulas: 
	- *G = Y + Cg* 
	- *R = Y – Cg + Co*
	- *B = Y – Cg – Co* 
	
	Require the following to work when Y, Co and Cg are each 8 bits: 
    - Y is treated as unsigned
    - Co and Cg are treated as signed
    - At least 10-bit precision is used for addition and subtraction operations

- The encoding formulas as they appear in the paper: 
	- *Y = G / 2 + (R + B) / 4* 
	- *Co = R - B* 
	- *Cg = G - (R + B) / 2* 
	
	Are not usable as written, because the next sentence says: 
    
    *"which is identical to the original definition, but with Co and Cg scaled up by a factor of two"* 
    
    Which means we really have:
	- *Y = G / 2 + (R + B) / 4* 
	- *Co = (R - B) / 2* 
	- *Cg = G / 2 - (R + B) / 4* 
	
    While these work, they don't work as well as they could when doing integer divides because of chop rounding, to fix this we make the following changes:
    - *Y = (R + (G * 2) + B + 2) / 4* 
	- *Co = (R - B + 1) / 2* 
	- *Cg = ((G * 2) - R - B + 2) / 4* 

    While this generally helps, there are 256 cases where r is 255 and b is zero and r swaps with b after encoding and then decoding, so we remove the adjustment on Co:
    - *Y = (R + (G * 2) + B + 2) / 4* 
	- *Co = (R - B) / 2* 
	- *Cg = ((G * 2) - R - B + 2) / 4* 

	This fixes nearly all of the problems of doing the calculations as integers but leaves eactly one glaring outlier: RGB(0, 255, 0) becomes YCoCg(128, 0, 128) which in turn converts back to RGB(255, 0, 255), which is an especially bad result and is fixed by making the following adjustment:
    - *Y = (R + (G * 2) + B + 1) / 4* 
	- *Co = (R - B) / 2* 
	- *Cg = ((G * 2) - R - B + 1) / 4* 
	
	This results in RGB(0, 255, 0) becoming YCoCg(127, 0, 127) which in turn converts back to RGB(0, 254, 0), which is a much better result and doesn't seem to have any other negative effects overall

- The accuracy of the encoding and decoding formulas is such that after converting *RGB* first to *YCoCg* then back to *RGB* (with an output precision of 24-bits at each step) ~25% of the doubly converted RGB values are identical to the original values and the remaining ~75% differ from the original value by exactly one in exactly one color channel

## LICENSE
This software is licensed under the
[MIT License](https://opensource.org/licenses/MIT).
