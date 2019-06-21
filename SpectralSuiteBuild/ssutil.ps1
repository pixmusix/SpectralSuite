param( [switch]$build = $false )


filter updateFromBase {     

    # loading base file every time as we make mutable changes to this variable's contents
    $basePath = Get-Location | Join-Path -ChildPath "base.jucer"
    $baseProjucer = New-Object -TypeName XML
    $baseProjucer.Load($basePath)

    $pluginJucerXml = New-Object -TypeName XML
    $pluginJucerXml.Load($_)
    $projectName = $pluginJucerXml.JUCERPROJECT.name
    $projectDescription = $pluginJucerXml.JUCERPROJECT.pluginDesc
    $pluginCode = $pluginJucerXml.JUCERPROJECT.pluginCode    


    if(!$projectDescription) {
        Write-Error "You must provide a description for $projectName"
        return        
    }
    $baseProjucer.JUCERPROJECT.name = $projectName
    $baseProjucer.JUCERPROJECT.pluginDesc = $projectDescription
    $baseProjucer.JUCERPROJECT.pluginCode = $pluginCode
    $baseProjucer.JUCERPROJECT.MAINGROUP.name = $projectName

    # add source code to base project template    
    $projectSourceCode = $pluginJucerXml.JUCERPROJECT.MAINGROUP.GROUP | Where-Object { $_.Name -eq "Source" }           
    $ignoredResult = $baseProjucer.JUCERPROJECT.MAINGROUP.PrependChild( $baseProjucer.ImportNode($projectSourceCode, $true) )
        
    $output = $baseProjucer.save($_)      
}

function Build-Release {
    param($jucerFile, $version)

    $output = Invoke-WebRequest "http://andrewreeman.github.io/spectral_suite_publish.json"
    $json = $output.Content | ConvertFrom-Json
    
    # loading base file every time as we make mutable changes to this variable's contents
    $basePath = Get-Location | Join-Path -ChildPath "base.jucer"
    $baseProjucer = New-Object -TypeName XML
    $baseProjucer.Load($basePath)    
    $projectVersion = $baseProjucer.JUCERPROJECT.version    
    
    
    if($projectVersion -eq $json.version) {
        Write-Error "You have not updated the version"
        exit
    }
    
    $definesString = $baseProjucer.JUCERPROJECT.defines
    $versionCodeString = $definesString.split("`n") | Where-Object { $_.contains("VersionCode")}
    $projectVersionCode = $versionCodeString.split("=")[1]

    if($projectVersionCode -eq $json.code) {
        Write-Error "You have not updated the version code"
        exit
    }
        
    # recreate
    Projucer.exe --resave $jucerFile | Write-Host

    $solution = [IO.Path]::Combine($jucerFile, "..", "Builds", "VisualStudio*", "*.sln")
    $solution = Resolve-Path $solution        
    $projectName = (get-item $solution).BaseName

    $sharedCodeProject = $projectName + "_SharedCode"
    $vstLegacyProject = $projectName + "_VST"
    $vst3Project = $projectName + "_VST3"

    Write-Host "Building $solution"    
    devenv.com $solution /Clean "Release" /Project $sharedCodeProject | Write-Host    
    devenv.com $solution /Clean "Release" /Project $vstLegacyProject | Write-Host
    devenv.com $solution /Clean "Release" /Project $vst3Project | Write-Host    
    devenv.com $solution /Clean "Release32|Win32" /Project $sharedCodeProject | Write-Host
    devenv.com $solution /Clean "Release32|Win32" /Project $vstLegacyProject | Write-Host   

    echo "Building shared code 64 bit project for $projectName"
    devenv.com /Build "Release" /Project $sharedCodeProject $solution | Write-Host
    
    echo "Building vst2 64 bit project for $projectName"
    devenv.com /Build "Release" /Project $vstLegacyProject $solution | Write-Host
    
    echo "Building vst3 64 bit project for $projectName"
    devenv.com /Build "Release" /Project $vst3Project  $solution | Write-Host    

    echo "Building shared code 32 bit project for $projectName"
    devenv.com /Build "Release32|Win32" /Project $sharedCodeProject $solution | Write-Host    
    
    echo "Building vst2 32 bit project for $projectName"
    devenv.com /Build "Release32|Win32" /Project $vstLegacyProject $solution | Write-Host    

    $releaseDir = [IO.Path]::Combine($jucerFile, "..", "..", "Release")
    $releaseDir = Resolve-Path $releaseDir
    
    $vst64 = [IO.Path]::Combine($jucerFile, "..", "Builds", "VisualStudio*", "x64", "Release", "VST", "*.dll")        
    $vst64Target = [IO.Path]::Combine($releaseDir, $projectName + ".dll")
    cp $vst64 $vst64Target

    $vst3_64 = [IO.Path]::Combine($jucerFile, "..", "Builds", "VisualStudio*", "x64", "Release", "VST3", "*.vst3")        
    $vst3_64Target = [IO.Path]::Combine($releaseDir, $projectName + ".vst3")
    cp $vst3_64 $vst3_64Target

    $vst32 = [IO.Path]::Combine($jucerFile, "..", "Builds", "VisualStudio*", "Win32", "Release32", "VST", "*.dll")    
    $vst32Target = [IO.Path]::Combine($releaseDir, $projectName + "_Win32.dll")
    cp $vst32 $vst32Target    
}





$pluginJucerPath = Get-Location | Join-Path -ChildPath "../*/*.jucer"
$pluginJucerPath = Resolve-Path -Path $pluginJucerPath
$pluginJucerPaths = $pluginJucerPath | where {$_.ToString() -notlike "*SpectralSuiteBuild*"}

$errors = $pluginJucerPaths | updateFromBase
if($errors) {
    echo $errors
    return
}

echo "Updated projucer files"

if ($build) {
    $basePath = Get-Location | Join-Path -ChildPath "base.jucer"
    $baseProjucer = New-Object -TypeName XML
    $baseProjucer.Load($basePath)
    $version = $baseProjucer.JUCERPROJECT.version

    ForEach ($pluginPath in $pluginJucerPaths) {
        Build-Release $pluginPath $version
    }

    echo "Built release files"
}