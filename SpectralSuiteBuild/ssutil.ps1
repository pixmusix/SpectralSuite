param( [switch]$build = $false )


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
    $vst3Project = $projectName + "_VST3"

    Write-Host "Building $solution"    
    devenv.com $solution /Clean "Release" /Project $sharedCodeProject | Write-Host        t
    devenv.com $solution /Clean "Release" /Project $vst3Project | Write-Host        

    echo "Building shared code 64 bit project for $projectName"
    devenv.com /Build "Release" /Project $sharedCodeProject $solution | Write-Host    
    
    echo "Building vst3 64 bit project for $projectName"
    devenv.com /Build "Release" /Project $vst3Project  $solution | Write-Host    
        

    $releaseDir = [IO.Path]::Combine($jucerFile, "..", "..", "Release")
    $releaseDir = Resolve-Path $releaseDir    

    $vst3_64 = [IO.Path]::Combine($jucerFile, "..", "Builds", "VisualStudio*", "x64", "Release", "VST3", "*.vst3")        
    $vst3_64Target = [IO.Path]::Combine($releaseDir, $projectName + ".vst3")
    cp $vst3_64 $vst3_64Target
}


$pluginJucerPath = Get-Location | Join-Path -ChildPath "../*/*.jucer"
$pluginJucerPath = Resolve-Path -Path $pluginJucerPath
$pluginJucerPaths = $pluginJucerPath | where {$_.ToString() -notlike "*SpectralSuiteBuild*"}


$basePath = Get-Location | Join-Path -ChildPath "base.jucer"
$baseProjucer = New-Object -TypeName XML
$baseProjucer.Load($basePath)
$version = $baseProjucer.JUCERPROJECT.version

ForEach ($pluginPath in $pluginJucerPaths) {
    Build-Release $pluginPath $version
}

echo "Built release files"
