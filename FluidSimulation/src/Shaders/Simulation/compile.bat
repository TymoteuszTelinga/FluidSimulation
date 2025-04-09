C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e ExternalForce --source-entrypoint main -o ExternalForce.spv simulation2D.comp
C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e DensityStep --source-entrypoint main -o DensityStep.spv simulation2D.comp
C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e PressureStep --source-entrypoint main -o PressureStep.spv simulation2D.comp
C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e ViscosityStep --source-entrypoint main -o ViscosityStep.spv simulation2D.comp
C:\VulkanSDK\1.4.304.1\Bin\glslangValidator -V -e UpdateStep --source-entrypoint main -o UpdateStep.spv simulation2D.comp

C:\VulkanSDK\1.4.304.1\Bin\spirv-link ExternalForce.spv DensityStep.spv PressureStep.spv ViscosityStep.spv UpdateStep.spv -o simulation2D.spv
del "ExternalForce.spv"
del "DensityStep.spv"
del "PressureStep.spv"
del "ViscosityStep.spv"
del "UpdateStep.spv"
pause