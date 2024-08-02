---

### This is CPlantBox with the installation script for BonnaHPC

This is a stabled forked version of CPlantBox/master Aug-2024 
- https://github.com/Plant-Root-Soil-Interactions-Modelling/CPlantBox/tree/1089fe0421e24e834f69720598b0789cc7079148

To install in **BonnaHPC** use the script below:

```bash
wget https://raw.githubusercontent.com/Murilodsv/CPlantBox/master/installCPlantBoxBonna.py

module load Python/3.8.2-GCCcore-9.3.0
module load VTK/8.2.0-foss-2020a-Python-3.8.2
module load CMake/3.16.4-GCCcore-9.3.0

python3 installCPlantBoxBonna.py
```

Examples are provided in "CPlantBox/tutorial/examples/". For instance:
```bash
cd CPlantBox/tutorial/examples/
python3 example1a_small.py
```

Further details in the main repository:
- https://github.com/Plant-Root-Soil-Interactions-Modelling/CPlantBox


