@Library("PSL") _
    
def axisNode = ["GEC-vs2017", "GEC-xcode1021"]

def tasks = [:]

for(int i=0; i< axisNode.size(); i++) {
    def axisNodeValue = axisNode[i]
    tasks["${axisNodeValue}"] = {
        node(axisNodeValue) {
            def components = env.JOB_NAME.split('/')
            env.COMPONENT = components[1].toLowerCase()
            def branch = "${env.BRANCH_NAME}"
            def WorkDir
            def WorkDirComp
            if(axisNodeValue.contains("GEC-vs")) {
                WorkDir="D:\\Stage\\workspace\\${env.COMPONENT}\\" + env.BRANCH_NAME.replace("/", "\\")
                WorkDirComp = WorkDir + "\\${env.COMPONENT}"
            } else {
                WorkDir="/Stage/workspace/${env.COMPONENT}/${env.BRANCH_NAME}"
                WorkDirComp = WorkDir + "/${env.COMPONENT}"
            }
            try {
                ws("${WorkDirComp}"){
                    stage("Sync") {
                        println "Node=${env.NODE_NAME}"
                    
                        checkout([$class: 'GitSCM', branches: scm.branches, doGenerateSubmoduleConfigurations: false, extensions: [[$class: 'SubmoduleOption', disableSubmodules: false, parentCredentials: true, recursiveSubmodules: true, reference: '', trackingSubmodules: false]], submoduleCfg: [], userRemoteConfigs: scm.userRemoteConfigs])
                        if(axisNodeValue.contains("GEC-vs")) {
                            if(branch.contains("release")) {
                                map = readProperties file: "adsk-build-scripts\\adsk-contrib\\release.properties"
                            } else {
                                echo "This is a non-release branch"
                                map = readProperties file: "adsk-build-scripts\\adsk-contrib\\dev.properties"
                            }
                        } else {
                            if(branch.contains("release")) {
                                map = readProperties file: "adsk-build-scripts/adsk-contrib/release.properties"
                            } else {
                                echo "This is a non-release branch"
                                map = readProperties file: "adsk-build-scripts/adsk-contrib/dev.properties"
                            }
                        }
                        properties = map.collect { key, value -> return key+'='+value }
                        
                        def sha = getCommitSha(axisNodeValue,WorkDirComp).trim()
                        properties.add("GITCOMMIT=${sha}")
                    }
                    stage("Build") {
                        println (properties)
                        if(axisNodeValue.contains("GEC-vs")) {
                            bat "git clean -fdx"
                            bat '''
                            set cmake=C:\\Users\\svc_airbuild\\.jenny\\tools\\cmake\\3.7.1\\bin\\cmake
                            %cmake% -S . -B_mtlxbuild -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=%WORKSPACE%\\install -DMATERIALX_BUILD_PYTHON=OFF  -DMATERIALX_BUILD_RENDER=ON -DMATERIALX_BUILD_TESTS=OFF -DMATERIALX_DEBUG_POSTFIX=d  -DMATERIALX_BUILD_GEN_OSL=ON -DMATERIALX_BUILD_GEN_MDL=OFF -DMATERIALX_BUILD_GEN_OGSXML=OFF -DMATERIALX_BUILD_GEN_OGSFX=OFF -DMATERIALX_BUILD_GEN_ARNOLD=ON -DMATERIALX_BUILD_CONTRIB=OFF -DMATERIALX_BUILD_VIEWER=OFF -DMATERIALX_INSTALL_INCLUDE_PATH=inc -DMATERIALX_INSTALL_LIB_PATH=libs -DMATERIALX_INSTALL_STDLIB_PATH=libraries
                            %cmake% --build _mtlxbuild --config Debug
                            %cmake% --build _mtlxbuild --target install
                            %cmake% --build _mtlxbuild --config Release
                            %cmake% --build _mtlxbuild --config Release --target install
                            '''
                        } else {
                            sh "git clean -fdx"
                            sh '''
                            cmake -S . -B_mtlxbuild -G "Xcode" -DCMAKE_INSTALL_PREFIX=$WORKSPACE/install -DMATERIALX_BUILD_PYTHON=OFF -DMATERIALX_BUILD_RENDER=ON -DMATERIALX_BUILD_TESTS=OFF -DMATERIALX_DEBUG_POSTFIX=d -DMATERIALX_BUILD_GEN_OSL=ON -DMATERIALX_BUILD_GEN_MDL=OFF -DMATERIALX_BUILD_GEN_OGSXML=OFF -DMATERIALX_BUILD_GEN_OGSFX=OFF -DMATERIALX_BUILD_GEN_ARNOLD=ON -DMATERIALX_BUILD_CONTRIB=OFF -DMATERIALX_BUILD_VIEWER=OFF -DMATERIALX_INSTALL_INCLUDE_PATH=inc -DMATERIALX_INSTALL_LIB_PATH=libs -DMATERIALX_INSTALL_STDLIB_PATH=libraries
                            cmake --build _mtlxbuild --config Debug
                            cmake --build _mtlxbuild --target install
                            cmake --build _mtlxbuild --config Release
                            cmake --build _mtlxbuild --config Release --target install
                            '''
                        }
                    }

                    stage("Create Nuget Packages") {
                        withEnv(properties) {
                            if(branch.contains("release")) {
                                properties.add("NUGET_VERSION=${env.Version}")
                            } else {
                                properties.add("NUGET_VERSION=1.0.0-${env.GITCOMMIT}")
                            }
                        }
                        withEnv(properties) {
                            if(axisNodeValue.contains("GEC-vs")) {
                                bat """
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-headers_win.nuspec -Version %NUGET_VERSION% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-lib_win_debug_intel64.nuspec -Version %NUGET_VERSION% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-lib_win_release_intel64.nuspec -Version %NUGET_VERSION% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-content.nuspec -Version %NUGET_VERSION% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                nuget pack adsk-build-scripts\\nuget\\win\\adsk_materialx-sdk_win_intel64.nuspec -Version %NUGET_VERSION% -OutputDirectory %WORKSPACE%\\packages -Prop installdir=%WORKSPACE%\\install -Prop materialx=${materialx_version} -Prop win_compiler=${win_compiler}
                                """
                                zip zipFile: "${env.WORKSPACE}/packages/adsk_materialx-sdk_win_intel64.${env.NUGET_VERSION}.zip", dir: "${env.WORKSPACE}/install", glob: '**/*'
                            } else {
                                sh """
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-headers_osx.nuspec -Version $NUGET_VERSION -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-lib_osx_debug_intel64.nuspec -Version $NUGET_VERSION -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-lib_osx_release_intel64.nuspec -Version $NUGET_VERSION -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-content.nuspec -Version $NUGET_VERSION -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                nuget pack adsk-build-scripts/nuget/osx/adsk_materialx-sdk_osx_intel64.nuspec -Version $NUGET_VERSION -OutputDirectory $WORKSPACE/packages -Prop installdir=$WORKSPACE/install -Prop materialx=${materialx_version} -Prop osx_compiler=${osx_compiler} -Prop osx_target=${osx_target}
                                """
                                zip zipFile: "${env.WORKSPACE}/packages/adsk_materialx-sdk_osx_intel64.${env.NUGET_VERSION}.zip", dir: "${env.WORKSPACE}/install", glob: '**/*'
                            }
                        }
                    }
                    stage("Upload Artifactory") {
                        withEnv(properties) {
                            uploadArtifactory()
                        }
                    }
                }
            } catch (caughtError) {
                println ("Error: " + caughtError)
                currentBuild.result="FAILURE"
            }
        }
    }
}

stage("Build") {
    parallel tasks
}
stage ("Update Version") {
    node("GEC-vs2017") {
        if(!currentBuild.currentResult.contains("FAILURE")) {
            if ("${env.BRANCH_NAME}".contains("release")){
                checkout scm
                withEnv(["file=adsk-build-scripts\\adsk-contrib\\release.properties"]) {
                    withEnv(["branch=${env.BRANCH_NAME}"]) {
                    bat '''
                        git checkout %branch%
                        git pull
                        pushd adsk-build-scripts
                        python versioning.py -u -b release
                        popd
                        git commit %file% -m "Update build version"
                        git push origin %branch%
                    '''
                    }
                }
            }
        }
    }
}

def getCommitSha(axisNodeValue,WorkDirComp){
    if(axisNodeValue.contains("GEC-vs")) {
        def sha = bat(
              returnStdout: true,
              script: """
                    @echo off
                    git rev-parse --short HEAD
                """
            ).trim()
        return sha
    } else {
        return sh(returnStdout: true, script: 'git rev-parse --short HEAD')
    }
}


def uploadArtifactory() {
    echo "INFO: Upload to Artifactory (Nuget)"
    withCredentials([usernamePassword(credentialsId: "832c4de4-8264-4b99-856e-e1d2b5dc2ffc", passwordVariable: 'PASSWORD', usernameVariable: 'USERNAME')]) {
    def server = Artifactory.newServer url: 'https://art-bobcat.autodesk.com/artifactory/', username: "${USERNAME}", password: "${PASSWORD}"
        def uploadSpec = """{
            "files": [{
                "pattern": "${env.WORKSPACE}/packages/*.nupkg",
                "target": "team-gfx-nuget/materialx/${env.NUGET_VERSION}/",
                "recursive": "false",
                "props":"git.branch=${env.BRANCH_NAME};git.hash=${env.GITCOMMIT}"
            }]
        }"""
        server.upload(uploadSpec)
    }
    echo "INFO: Upload to Artifactory (Zip)"
    withCredentials([usernamePassword(credentialsId: "832c4de4-8264-4b99-856e-e1d2b5dc2ffc", passwordVariable: 'PASSWORD', usernameVariable: 'USERNAME')]) {
    server = Artifactory.newServer url: 'https://art-bobcat.autodesk.com/artifactory/', username: "${USERNAME}", password: "${PASSWORD}"
         uploadSpec = """{
            "files": [{
                "pattern": "${env.WORKSPACE}/packages/*.zip",
                "target": "team-gfx-generic/materialx/${env.NUGET_VERSION}/",
                "recursive": "false",
                "props":"git.branch=${env.BRANCH_NAME};git.hash=${env.GITCOMMIT}"
            }]
        }"""
        server.upload(uploadSpec)
    }   
}
