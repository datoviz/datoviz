git clone --recursive git@github.com:viskydev/visky.git
cd visky
./manage.sh build
cp data/swiftshader/linux/* build/
export VK_ICD_FILENAMES=$(pwd)/build/vk_swiftshader_icd.json
./build/visky test
