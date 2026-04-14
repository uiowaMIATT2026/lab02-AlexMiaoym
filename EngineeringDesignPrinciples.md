Step 1: Build the environment
Set up the CMake environment and make sure the ITK package is properly configured and usable.

Step 2: Edit the C++ file
Implement the registration part of the code. Test it using two random images to ensure the program runs correctly.

Step 3: Verify the code
Use a specific test image (e.g., a circle image) to verify that the algorithm works as expected.

-------------
*Step 1.
According to the hw structure, 

lab2-build
    -src(register.txt)
    -CMakeLists.txt

make this folder structure

-------------
*Step 2.
1) According to the ITK second Book, Find the example/ImageRegistration.cxx file
2) Implant the Code, I Can run the registration program now
 ./src/ece5940_lab02 $DATA/Circle.png $DATA/Input.png out.png
 I need read the code and unserstand the code. (Need add some resriction)

-------------
*Step 3.
1) add some restriction for fixed image and moving image
write the code to creat the image.

folder structure
lab2-build
    -src
        - createimage(CreateImage.cxx)
        - register(ImageRegistration.cxx)
    -CMakeLists.txt

2) modify the register.cxx to fit the creating image.

3) use the creating image to verfy the code.


------------- 
Algorithm
1) Create Image
command: createimage fixed.png diameter(mm)  center(mm)

Set the image grid size to 512X512 pixels adn the physical spacing to 0.5 mm per pixel.

Calculate the radius from the diameter and square it. 

Save it to disk as a png.file


2) Registration
command: register fixed.png moving.png output.png

Initialization: Load the "Fixed" and "Moving" images. Assign a physical coordinate system (0.5 mm spacing) to establish real-world alignment, which standard image files inherently lack.

Component Setup: Wire together four core ITK tools: a TranslationTransform to shift the image, a MeanSquares metric to calculate the alignment error, Linear interpolators for sub-pixel accuracy, and a GradientDescent optimizer to drive the process.

Initial Alignment: Apply a pre-calculated mathematical offset so the fixed and moving shapes initially overlap. Without this jump-start, the shapes wouldn't touch, the error gradient would be completely flat (comparing only black background pixels), and the optimizer would fail to move.

Optimization Loop: The optimizer iteratively shifts the moving image down the metric's gradient. It evaluates the mean squares difference, adjusts its step size dynamically, and repeats until the error is minimized.

Resampling: Combine the starting offset with the newly optimized translation. A resampler then warps the moving image, painting its pixels onto the fixed image's exact grid layout.

Validation: Mathematically subtract the aligned moving image from the fixed image. The resulting difference map is saved to visually prove the success of the alignment.
