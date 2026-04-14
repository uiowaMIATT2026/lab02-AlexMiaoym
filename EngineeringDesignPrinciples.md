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


2) Registration
command: register fixed.png moving.png output.png








