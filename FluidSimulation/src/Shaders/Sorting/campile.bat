C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e BitonicSort --source-entrypoint main -o BitonicSort.spv bitonicSort.comp
C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e CalculateOffsets --source-entrypoint main -o CalculateOffsets.spv bitonicSort.comp

C:\VulkanSDK\1.4.304.1\Bin\spirv-link BitonicSort.spv CalculateOffsets.spv -o Sort.spv
del "BitonicSort.spv"
del "CalculateOffsets.spv"
pause