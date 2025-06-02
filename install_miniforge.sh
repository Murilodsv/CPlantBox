#!/bin/bash
wget -O Miniforge3.sh "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-$(uname)-$(uname -m).sh"
bash Miniforge3.sh -b -f
rm Miniforge3.sh

source "${HOME}/miniforge3/etc/profile.d/conda.sh"
conda activate

if [ $? -eq 0 ]
then
  echo "Miniforge succesfully installed"
else
  echo "ERROR: Miniforge3 could not be installed"
  exit 1
fi
