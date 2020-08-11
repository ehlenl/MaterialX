import re
import os
import sys
import json
import requests
import argparse
import tempfile
from requests.auth import HTTPBasicAuth

VERSION_FILE_DIR = ""

def increment_one_version(filename, key):
    with open(filename, 'rU') as fin, tempfile.NamedTemporaryFile('w', dir=os.path.dirname(filename), delete=False) as fout:
        for line in fin.readlines():
            if line.startswith(key):
                prevversion = line.split('=')[1].split('.')
                newversion = prevversion[0] + "." + prevversion[1] + "." +  str(int(prevversion[2])+1)
                line = '='.join((line.split('=')[0], '{}'.format(newversion))) + "\n"
            fout.write(line)
        fin.close()
        fout.close()
    # remove old version
    os.unlink(filename)
    # rename new version
    os.rename(fout.name, filename)
    return
    
def increase_component_version():
    if os.environ['component']=='airmax-common':
                os.environ['component'] = 'airmaxcommon'
    increment_one_version(os.environ['branch']+".properties", os.environ['component']+"_version")
    return


def get_latest_version(key):
    if os.path.exists(VERSION_FILE_DIR+os.environ['branch']+".properties"):
        string=[key,os.environ['branch'],"*"]
    else:
        string=[key,"master","*"]
    string='/'.join(string)
    url = "https://art-bobcat.autodesk.com/artifactory/api/search/aql"
    response = requests.post(
    url, auth=HTTPBasicAuth(os.environ['username'],os.environ['apikey']), data='items.find({"repo":{"$eq":"team-gfx-nuget"},"path" : {"$match" : "' + string + '"},"type" : "file"}).sort({"$desc":["modified"]}).limit(1)')
    content = json.loads(response.text)
    if (content['results']==[]):
        return "latest"
    else:
        diction=content['results'][0]
        latestVersion=diction['path'].split('/')[-1]
        return latestVersion


def  replace_all_latest_version(filename):
    with open(filename, 'rU') as fin, tempfile.NamedTemporaryFile('w', dir=os.path.dirname(filename), delete=False) as fout:
        for line in fin.readlines():
            prevversion = line.split('=')[1].strip()
            key = line.split('=')[0].strip().split("_")[0]
            if key=='airmaxcommon':
                key = 'airmax-common'
            if prevversion=='latest':    
                newversion = get_latest_version(key)
                line = '='.join((line.split('=')[0], '{}'.format(newversion))) + "\n"
            fout.write(line)
        fin.close()
        fout.close()
    # remove old version
    os.unlink(filename)
    # rename new version
    os.rename(fout.name, filename)
    return
                                
def update_branch_properties():
    if os.path.exists(VERSION_FILE_DIR+os.environ['branch']+".properties"):
        replace_all_latest_version(VERSION_FILE_DIR+os.environ['branch']+".properties")
    else:
        replace_all_latest_version(VERSION_FILE_DIR+"master"+".properties")
    return                      
                               
                                
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-u",
                        "--update",
                        action="store_true",
                        help="Increase version in branch properties file.",
                        default=False)                  
    parser.add_argument("-b",
                        "--branch",
                        nargs=1,
                        help="Specify branch name for properties file",
                        required=True)
    parser.add_argument("-c",
                        "--component",
                        nargs=1,
                        help="Specify component name",
                        required=True)
    parser.add_argument("-n",
                        "--username",
                        nargs=1,
                        help="Specify username",
                        required=True)
    parser.add_argument("-a",
                        "--apikey",
                        nargs=1,
                        help="Specify api key",
                        required=True)
    parsed_args = parser.parse_args()
    
    os.environ['branch'] = parsed_args.branch[0]    
    os.environ['component'] = parsed_args.component[0]
    os.environ['username'] = parsed_args.username[0]
    os.environ['apikey'] = parsed_args.apikey[0]

    
    if ("ReleaseMajor" in os.environ['branch'] or "release/" in os.environ['branch']):
        increase_component_version()
    else:
        update_branch_properties()
