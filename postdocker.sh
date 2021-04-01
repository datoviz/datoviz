git clone --recursive https://github.com/datoviz/datoviz.git
cd datoviz
./manage.sh build
cp data/swiftshader/linux/* build/
export VK_ICD_FILENAMES=$(pwd)/build/vk_swiftshader_icd.json
./build/datoviz test
