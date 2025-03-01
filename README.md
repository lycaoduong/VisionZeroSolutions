# VisionZeroSolutions
 NX Witness Hackathon 2025

## Prerequisites

Ensure you have the following libraries and tools installed before proceeding:

### For Building Plugin
1. **Visual Studio 2019**  
- Download and install Visual Studio 2019: [Visual Studio 2019](https://visualstudio.microsoft.com/vs/older-downloads/).

2. **VSCode + CMake**
- Install Visual Studio Code (VSCode) from: [Visual Studio Code.](https://code.visualstudio.com/)
- CMake and CMake Tools extension for VSCode are required for building and managing CMake projects.
- Install the CMake extension for VSCode from the Extensions marketplace.

3. **CUDA 11.8**
- Download and install CUDA 11.8 for Windows: [CUDA 11.8](https:<>/).

4. **CuDNN**
- Download CuDNN for CUDA 11.8: [CuDNN](https:<>/).
- Once you've downloaded CuDNN, extract the contents and copy the following folders to the CUDA installation directory:
  ```
  lib → C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\lib\x64
  bin → C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\bin
  include → C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\include

### For Testing Plugin
1. **NX Server and NX Client for Windows**
   - Download and install NX Server and NX Client for Windows: [NX Server and NX Client.](https:<>/)

2. **VisionZero Team Plugin**
- Download VisionZero Team Plugin from: [nx_plugin_clip.](https:<>/)
- Paste the `nx_plugin_clip` folder into the plugins folder (After install NX Server)
   ```
   C:\Program Files\Network Optix\Nx Witness\MediaServer\plugins

3. **Docker Desktop**
- Install Docker Desktop from: [Docker Desktop.](https://www.docker.com/get-started/)
- Follow the installation instructions on the page to set up Docker on your system.

4. **MariaDB**
- Install MariaDB from: [MariaDB.](https:<>/)

5. **FTP Server**
- Download and run an FTP Server from: [FTP Server.](https:<>/)

6. **CLIP-Textual Server**
- Download and run the CLIP-Textual Server from: [CLIP-Textual Server.](https:<>/)

7. **HuggingFace lycaoduong/InternVL2-1B-NXDemo Space access token**
- To gain access to the HuggingFace space for InternVL2-1B-NXDemo, contact via email to get the access token: hf_<your token here> at lycaoduong@gmail.com.


## Installation and Build Steps

### Step 1: Download and Prepare Dependencies
- Download the required libraries (lib) Folder (includes `curl-7.86.0`, `eigen`, `mysql-8.0.32`, `onnxruntime`, `opencv`, `rapidjson`, and `spdlog-1.x`) and place them into the $(SolutionDir).
- Download nx_kit and nx_sdk folders and place them into the $(SolutionDir).
- Download and replace the latest Core (LVisLib.lib) and place them into the $(SolutionDir)core\LVisLib\lib folder.

### Step 2: Set Up the CMake Configuration
- Open the folder $(SolutionDir) in VS Code.

### Step 3: Configure the Build Kit
- Press Ctrl + Shift + P and search for CMake: Select a Kit.
- Choose Visual Studio 2019 (ensure that you select the x64 variant).

### Step 4: Configure and Build Using CMake
- Press Ctrl + Shift + P, search for CMake: Configure, and select it to configure the build.
- Press Ctrl + Shift + P, search for CMake: Select Build Variant, and select Release.
- After configuration is complete, press Ctrl + Shift + P again and search for CMake: Build
- Alternatively, you can run the build manually from the terminal by using the following command:
  ```
  cmake --build build --config Release

### Step 5: Deploy the Plugin
- Copy the generated `nx_plugin_clip.dll` file from the build output directory:
  ```
  $(SolutionDir)\build\Release\

### Step 6: Deploy the Plugin
- Paste the `nx_plugin_clip.dll` file into the NX Plugin Server folder
  ```
  C:\Program Files\Network Optix\Nx Witness\MediaServer\plugins\nx_plugin_clip
- Note: Stop the NX Server before overwriting the existing plugin file.

## Testing Steps

### Step 1: Run MariaDB Server
- Run the MariaDB Server by executing <MariaDB>.exe. (Download exe here: )

### Step 2: Run FTP Server
- Run the FTP Server by executing <ftpServer>.exe. (Download exe here: )

### Step 3: Run CLIP-Textual Server
- Run the CLIP-Textual Server by executing <CLIP-Textual Server>.exe. (Download exe here: )

### Step 4: Configure MariaDB, FTP, CLIP-Textual Server, and HuggingFace Server
- Open the nx_plugin_clip.ini configuration file and set the necessary parameters like IP address, port, and access token for each service.
- The nx_plugin_clip.ini file is located in the plugin folder:
  ```
  C:\Program Files\Network Optix\Nx Witness\MediaServer\plugins\nx_plugin_clip

### Step 5: Run Web App Searching by Docker Compose
- Start the container with docker-compose.yml to deploy the web application:
  ```
  docker-compose up -d

### Step 6: Start Server and Enable Plugin
- Enable the plugin on the desired camera through the Nx Witness interface.
- Add the actions for detection from the UI settings.

### Step 7: Search by Description
- Open the Vector Search WebApp in your browser at:
  ```
  http://localhost:7860
- Input a description (e.g., man in black clothes) to search and get the TOP-5 matching images.

## Supplementary

### Testing Video
- You can download the testing videos from the following link: <>

### Presentation and Demo Video
- Watch the presentation video here: <>

## Models + Resources

1. **YOLOv8 COCO**  
 - https://docs.ultralytics.com/ko/models/yolov8/

2. **CLIP** 
 - https://github.com/openai/CLIP 

3. **ByTrack**
 - https://github.com/ifzhang/ByteTrack  

4. **InternVL2.5-1B**  
 - https://github.com/OpenGVLab/InternVL

5. **Thief Video**  
 - Laoode, "Thief Video," [GitHub Repository](https://github.com/Laoode/Theft_Detection/tree/main/Test%20Videos)

 ## Citations
  ```BibTeX
  @inproceedings{radford2021learning,
  title={Learning transferable visual models from natural language supervision},
  author={Radford, Alec and Kim, Jong Wook and Hallacy, Chris and Ramesh, Aditya and Goh, Gabriel and Agarwal, Sandhini and Sastry, Girish and Askell, Amanda and Mishkin, Pamela and Clark, Jack and others},
  booktitle={International conference on machine learning},
  pages={8748--8763},
  year={2021},
  organization={PmLR}
}
@inproceedings{zhang2022bytetrack,
  title={Bytetrack: Multi-object tracking by associating every detection box},
  author={Zhang, Yifu and Sun, Peize and Jiang, Yi and Yu, Dongdong and Weng, Fucheng and Yuan, Zehuan and Luo, Ping and Liu, Wenyu and Wang, Xinggang},
  booktitle={European conference on computer vision},
  pages={1--21},
  year={2022},
  organization={Springer}
}
 @inproceedings{chen2024internvl,
  title={Internvl: Scaling up vision foundation models and aligning for generic visual-linguistic tasks},
  author={Chen, Zhe and Wu, Jiannan and Wang, Wenhai and Su, Weijie and Chen, Guo and Xing, Sen and Zhong, Muyan and Zhang, Qinglong and Zhu, Xizhou and Lu, Lewei and others},
  booktitle={Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition},
  pages={24185--24198},
  year={2024}
}
@misc{doan2024vintern1befficientmultimodallarge,
      title={Vintern-1B: An Efficient Multimodal Large Language Model for Vietnamese}, 
      author={Khang T. Doan and Bao G. Huynh and Dung T. Hoang and Thuc D. Pham and Nhat H. Pham and Quan T. M. Nguyen and Bang Q. Vo and Suong N. Hoang},
      year={2024},
      eprint={2408.12480},
      archivePrefix={arXiv},
      primaryClass={cs.LG},
      url={https://arxiv.org/abs/2408.12480}, 
}
```