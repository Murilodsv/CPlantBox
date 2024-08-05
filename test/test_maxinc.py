#! /usr/bin/python3

# @Testing RootSystem::simulate(double dt, double maxinc_, ProportionalElongation* se, bool verbose)
# m.vianna

#--- import libs
import sys
import numpy as np

#--- append CPlantBox module paths
sys.path.append('..')

#--- load CPlantBox
import plantbox as rb

#-------------------#
#--- sim configs ---#
#-------------------#
simtime = 150       # days
soildepth = 180     # cm
layers = 60         #
dt = 1              # days

#--- some root pars
rootparname = '../modelparameter/structural/rootsystem/Zea_mays_3_Postma_2011.xml'

#--- Initialize root system
rs = rb.RootSystem()
rs.setSeed(0)
rs.readParameters(rootparname) 

#--- Set scale elongaton according to tutorial example "example5b_scaleelongation.py"
scale_elongation = rb.EquidistantGrid1D(0, -soildepth, layers)
scale_elongation.data = np.ones((layers-1,))
for p in rs.getRootRandomParameter():
    p.f_se = scale_elongation  # set scale elongation function

rs.initialize()  
ol = 0

# Simulation loop
for s in range(0,simtime):    

    # a dummy fixed max increment [this would be a dynamic model, e.g. simplace]    
    maxinc = 10.

    # vertical reduction e.g. soil strenght [assuming no reduction = 1]
    re_reduction = np.ones((layers-1,))
    
    # testing some re_reduction at steps 110-130
    if s>=110 and s<=130:
        re_reduction = re_reduction*0.1
    
    # update scale elongation
    scale_elongation = rb.EquidistantGrid1D(0,-soildepth, layers)     
    scale_elongation.data = np.array(re_reduction) # set scale elongation to re_reduction from simplace
    for p in rs.getRootRandomParameter():        
        p.f_se = scale_elongation
    
    # get scale elongation as a ProportionalElongation class [for rs.simulate() 4 args signature]
    se = rb.ProportionalElongation()
    se.setBaseLookUp(scale_elongation)
        
    inc = 0.0        
    if(maxinc > 0):
        
        # run CPlantBox if there's any root increment
        rs.simulate(dt, maxinc, se, True)

        l = np.sum(rs.getParameter('length'))
        inc =  l - ol
        ol = l
        
        print('increase was '+str(round(inc, 2)))
        if inc > maxinc:
            print('Warning: Root increment exceeds max increment\n')

rs.write("test_maxinc.vtp")
print('done')
