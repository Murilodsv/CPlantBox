from fipy import *
from fipy.meshes.nonUniformGrid1D import NonUniformGrid1D as Grid1D
from fipy.meshes.nonUniformGrid2D import NonUniformGrid2D as Grid2D
from fipy.tools import numerix as np
import sys; sys.path.append("../../.."); sys.path.append("../../../src/python_modules")
from xylem_flux import XylemFluxPython  # Python hybrid solver
from phloem_flux import PhloemFluxPython, GetTime
from mpi4py import MPI

from CellVariablemod import CellVariablemod
import plantbox as pb
import vtk_plot as vp
import math
import os
from io import StringIO
from datetime import datetime, timedelta

home_dir = os.getcwd()
dir_name = "/results"
dir_name2 = home_dir + dir_name
test = os.listdir(dir_name2)
for item in test:
    if item.endswith("10c.txt"):
        os.remove(os.path.join(dir_name2, item))
    if item.endswith("10c.vtk"):
        os.remove(os.path.join(dir_name2, item))
    if item.endswith("10c.vtp"):
        os.remove(os.path.join(dir_name2, item))
        
        
np.set_printoptions(threshold=sys.maxsize)

#comm = MPI.COMM_WORLD
#rank = comm.Get_rank()


class NullIO(StringIO):
    def write(self, txt):
       pass
       

####
#
# wheather data
#
####
def weatherData(t):
    t_ = round(t/(60*60))
    tmp = ((t_+ 12) % 24) + (t/(60*60) - t_)
    trapeze = max(0, min(1, min(tmp-11, 24-tmp))) 
    TairC =  20*trapeze+5
    TairK = TairC + 273.15
    RH = 0.5 + 0.4*(1-trapeze)# relative humidity
    Q = 900e-6*trapeze # mol quanta m-2 s-1 light, example from leuning1995
    
    

    es = 0.61078 * math.exp(17.27 * TairC / (TairC + 237.3)) 
    ea = es * RH 
    VPD = es - ea 

    weatherVar = {'TairK' : TairK,
                    'Qlight': Q,#(-)
                    'VPD': VPD}
    print('weather ', t,trapeze,  weatherVar)
    return weatherVar
    

    
    
######################
#
# plant
#
####################### 
pl = pb.MappedPlant() #pb.MappedRootSystem() #pb.MappedPlant()

path = "../../../modelparameter/plant/" #"../../../modelparameter/rootsystem/" 
name =  "Triticum_aestivum_adapted_2021"#"smallPlant_mgiraud"#"morning_glory_7m"#"manyleaves"#"oneroot_mgiraud" #"manyleaves"
pl.readParameters(path + name + ".xml")
start =35

""" Parameters xylem"""
kz = 4.32e-1  # axial conductivity [cm^3/day] 
kr = 1.728e-4  # radial conductivity of roots [1/day]
kr_stem = 1.e-20  # radial conductivity of stem  [1/day], set to almost 0
gmax = 0.004 #  cm3/day radial conductivity between xylem and guard cell
p_s = -200  # static water potential (saturation) 33kPa in cm
#p_g = -2000 # water potential of the guard cell
p_a =  -1000  #default outer water potential 
simtime = 15 # [day] for task b
k_soil = []
cs = 350e-6 #co2 paartial pressure at leaf surface (mol mol-1)
p_linit = -10000 #guess for first p_l value 




""" Parameters phloem """
""" soil """
min_ = np.array([-50, -50, -150])
max_ = np.array([90, 40, 10])
res_ = np.array([5, 5, 5])
pl.setRectangularGrid(pb.Vector3d(min_), pb.Vector3d(max_), pb.Vector3d(res_), True)  # cut and map segments
pl.initialize(True)

pl.simulate(start, False)



phl = PhloemFluxPython(pl)
phl.mp2mesh() #creates grid
organTypes = phl.get_organ_types()
ana = pb.SegmentAnalyser(phl.rs)
ana.write("results/example10a.vtp" )

cellsIDOld = [phl.new2oldNodeID[xi] - 1 for xi in phl.mesh.cellFaceIDs[1] ]
initphi =0.

initphi = 0.#np.loadtxt('Cfin.txt', skiprows=0)[1:]/1000/12 #mmol C to mol suc
#tip indexes 
tiproots, tipstem, tipleaf = phl.get_organ_nodes_tips()
tiproots_newID = np.array([phl.old2newNodeID[xi] for xi in tiproots]).flatten()
tiproots=  [np.any([tiproots_newID == yi]) for yi in phl.mesh.faceVertexIDs[0]]
tiprootsfaces= sum([phl.mesh.faceVertexIDs[0] == xi for xi in tiproots_newID]) 
tiprootsfacesID= np.where(tiprootsfaces)[0]
tiprootscells= sum([phl.mesh.cellFaceIDs[1] == xi for xi in tiprootsfaces])
tiprootscellsID= np.where(tiprootscells)[0]

####
#
# Equations
#
####

dt =  1#0.9 * min(phl.mesh.length) ** 2 / 2#(2 * max(phl.Source)) #set to seconds

phl.phi.updateOld()
growthSteps = []
issue = []
issueRes = []
issueLoop = []
phiConcentrationError = []


phl.oldCellsID = [phl.new2oldNodeID[xi] - 1 for xi in phl.mesh.cellFaceIDs[1] ]


cumulOut = CellVariable(mesh = phl.mesh, value=0.)
cumulGr = CellVariable(mesh = phl.mesh, value=0.)
cumulRm = CellVariable(mesh = phl.mesh, value=0.)
cumulAn = CellVariable(mesh = phl.mesh, value=0.)

timeSpanCheck = 60*10
timeSinceLastCheck = np.inf
numcheck = 0

organGr = [0.]

#for _ in range(steps):
step = 0
beginning = datetime.now()
simDuration = 0
dtVal = dt=0.125
alldts = np.array([])
ana = pb.SegmentAnalyser(phl.rs)
ana.write("results/example10c.vtp" , ["radius", "surface"])
maxdt = 60
increase = 1.5
    
phl.resetValues(simDuration, Leuning = False)    
        
while simDuration < 36*60*60: #
    end = datetime.now()
    duration = end - beginning
    print('\n\nstep n°', step,', dt: ',dt,'s, ','tot sim time: ', GetTime(simDuration), ', computation time: ',duration)
    
    convergence = False
    #print("js ",phl.Mesophyll,phl.Agross,  phl.JS_Mesophyll*dt,phl.Mesophyll+phl.JS_Mesophyll*dt-phl.Agross*dt)
    if(step > 0 ): #convergence criteria according to phi value before loop (as might get odd values during loop)
        phi4res = phl.phi.copy() - initphi
    else: #for first loop, phi == 0, so set phi4res as CellVariable => can update at each loop (otherwise convergence never reached)
        phi4res = phl.phi - initphi
    while (not convergence):
        eqMesophyll = (TransientTerm(var = phl.Mesophyll) == phl.Agross - phl.JS_Mesophyll)
        eq = (TransientTerm(var = phl.phi) ==  (phl.phi.faceValue * phl.phi.faceGrad*phl.intCoeff ).divergence + phl.JS_Mesophyll -phl.totalSinkFactor)#+ ImplicitSourceTerm(var = phl.phi,coeff=-phl.totalSinkFactor ))
        #eq2 = (TransientTerm(var = phl.SucSink) ==  phl.totalSinkFactor)#ImplicitSourceTerm(var = phl.phi,coeff=phl.totalSinkFactor ))
        
        eqt = eq  & eqMesophyll #& eq2
        res =diff= 1e+10
        resOld = 2e+10
        loop = 0
        #or diff /max(phl.phi* phl.mesh.cellVolumes)> 0.1 
        while ( ( res > 1e-5*max(phi4res* phl.mesh.cellVolumes)or np.isnan(res) or any(phl.phi <0)or any(phl.SucSink <0)) and loop <= 100 ) : #and resOld != res, or (diff3 /max(phl.phi* phl.mesh.cellVolumes)> 0.0001 )
            resOld = res
            res = eqt.sweep(dt= dt) #quid solver?
            #diff = abs(sum(phl.phi* phl.mesh.cellVolumes) -sum( phiOld* phl.mesh.cellVolumes)-sum(self.Agross*dt* phl.mesh.cellVolumes)\
             #   + sum(phl.phi * phl.totalSinkFactor*dt* phl.mesh.cellVolumes))
            #print('res: ',res, ' lim = ', 1e-3*max(phi4res* phl.mesh.cellVolumes))
          
            loop += 1
            
        if loop > 100:#:res > 1e-6*max(phl.phi* phl.mesh.cellVolumes) :
            print('no convergence! res: ', res,' lim = ', 1e-5*max(phi4res* phl.mesh.cellVolumes))
            issue = np.append(issue, [step])
            issueRes = np.append(issueRes, [res])
            issueLoop = np.append(issueLoop, [loop])
            dtVal = dtVal/2
            dt = dtVal
            maxdt = 1
            increase = 1.01
            print("change dt from ", dtVal*2, " to ", dtVal)
        else:
            print('convergence reached')
            convergence = True
            alldts = np.append(alldts, dt)
                
    #print(sum(phl.JS_Mesophyll* phl.mesh.cellVolumes*dt),sum(phl.Mesophyll* phl.mesh.cellVolumes+phl.JS_Mesophyll*dt* phl.mesh.cellVolumes-phl.Agross*dt* phl.mesh.cellVolumes))
    #print('check1 ',sum(phl.SucSink), sum(phl.outFlowMax))
    #phl.SucSink.updateOld()
    phl.phi.updateOld()
    phl.Mesophyll.updateOld()
    #print('jsmeso ', phl.JS_Mesophyll[np.where(phl.JS_Mesophyll>0)[0]]* phl.mesh.cellVolumes[np.where(phl.JS_Mesophyll>0)[0]]*3600*12*1000)
    #,sum(phl.Mesophyll* phl.mesh.cellVolumes+phl.JS_Mesophyll*dt* phl.mesh.cellVolumes-phl.Agross*dt* phl.mesh.cellVolumes))
    phl.getCLimitedSinks(phl.totalSinkFactor.value*dt, dt)# phl.SucSink.value - SucSinkOld.value, dt) #compute C-limited Rm, GrSink and outflow
    cumulRm.setValue(cumulRm.value +phl.Rm * phl.mesh.cellVolumes)
    cumulAn.setValue(cumulAn.value + phl.JS_Mesophyll* dt* phl.mesh.cellVolumes)#phl.Agross.value * dt* phl.mesh.cellVolumes)
    cumulGr.setValue(cumulGr.value + phl.GrSink * phl.mesh.cellVolumes)
    cumulOut.setValue(cumulOut.value + phl.outFlow * phl.mesh.cellVolumes)
    phl.SucSink.setValue(phl.SucSink.value +phl.totalSinkFactor*dt*phl.mesh.cellVolumes)
    phl.SucSink.updateOld()
    
    simDuration += dt
    timeSinceLastCheck +=dt
    
        
    ####
    #
    # check balance
    #
    ####
    #print('check if value updates2 ',sum(phl.totalSinkFactor), sum(phl.RmMax),  sum(phl.outFlowMax)) #check if value updates
    phl.doCheck(cumulAn, cumulOut,cumulRm,cumulGr, initphi, timeSinceLastCheck , timeSpanCheck, numcheck,issue, issueRes,  issueLoop,phiConcentrationError, dt)
    
    if timeSinceLastCheck >= timeSpanCheck: #choose how often get output 
        numcheck +=1
        end = datetime.now()
        duration = end - beginning
        phl.doLog( duration, organTypes, dt, issue, issueRes, issueLoop, alldts, simDuration) #print in log
        timeSinceLastCheck = 0.
        
    
    step += 1
    #if step ==10 :
     #   dtVal = 0.5
    
    if loop < 10:
        dtVal = min(dtVal*increase, maxdt)
    dt = dtVal#min(0.9 * min(phl.mesh.length)**2 / (2 * max(phl.phi.faceValue *phl.intCoeff)),1)# dtVal# 60*60#min((0.9 * min(phl.mesh.length) ** 2 / (2 * max((phl.phi.faceValue*phl.intCoeff).value)),30))
    
    phl.valueError() #raise exception if impossible values
    
    
end = datetime.now()
duration = end - beginning
print('duration simulation ', duration.seconds, 's for a simulation of ', step ,' steps and ', GetTime(simDuration),  "\n", dt)

#FiPy’s order of precedence when choosing the solver suite for generic solvers is Pysparse followed by Trilinos, PyAMG and SciPy.
            #and PyAMGX?
            #solver option with scipy:
            #DefaultSolver = LinearLUSolver
            #DummySolver = LinearGMRESSolver
            #DefaultAsymmetricSolver = LinearLUSolver
            #GeneralSolver = LinearLUSolver
            #__all__.extend(linearCGSSolver.__all__) => LinearCGSSolver => 3s
            #__all__.extend(linearGMRESSolver.__all__) => LinearGMRESSolver =>  3
            #__all__.extend(linearBicgstabSolver.__all__) => not defined
            #__all__.extend(linearLUSolver.__all__) => 3
            #__all__.extend(linearPCGSolver.__all__) => 3s
            #help for solver: https://www.mail-archive.com/fipy@nist.gov/msg01089.html
