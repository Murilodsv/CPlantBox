#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# find source
dic = GetSources();
keys = dic.keys();
sorghumvtp = FindSource(keys[0][0])

# create a new 'Cell Data to Point Data'
cellDatatoPointData1 = CellDatatoPointData(Input=sorghumvtp)

# Properties modified on cellDatatoPointData1
cellDatatoPointData1.PassCellData = 1

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# show data in view
cellDatatoPointData1Display = Show(cellDatatoPointData1, renderView1)
# trace defaults for the display properties.
cellDatatoPointData1Display.ColorArrayName = [None, '']

# hide data in view
Hide(sorghumvtp, renderView1)

# create a new 'Tube'
leaf= Threshold(Input=cellDatatoPointData1, Scalars="organtype", ThresholdRange=(7,9))
leaf_poly= ExtractSurface(leaf)

stem= Threshold(Input=cellDatatoPointData1, Scalars="organtype", ThresholdRange=(3,5))
stem_poly= ExtractSurface(stem)

root= Threshold(Input=cellDatatoPointData1, Scalars="organtype", ThresholdRange=(0,3))
root_poly= ExtractSurface(root)

leaf_ribbon = Ribbon(leaf_poly)
leaf_ribbon.Angle=90
leaf_ribbon.Scalars = 'id'
leaf_ribbon.UseDefaultNormal=1
leaf_ribbon.DefaultNormal=(1,1,0)
leaf_ribbon.Width=0.75
#print(isleaf)



stem_tube = Tube(Input=stem_poly)
stem_tube.Scalars = ['POINTS', 'radius']
stem_tube.Vectors = [None, '1']
stem_tube.Radius = 0.32146133262802323

# Properties modified on stem_tube
stem_tube.Vectors = [None, '']
stem_tube.VaryRadius = 'By Absolute Scalar'

# show data in view
tube1Display = Show(stem_tube, renderView1)

root_tube = Tube(Input=root_poly)
root_tube.Scalars = ['POINTS', 'radius']
root_tube.Vectors = [None, '1']
root_tube.Radius = 0.32146133262802323

# Properties modified on stem_tube
root_tube.Vectors = [None, '']
root_tube.VaryRadius = 'By Absolute Scalar'



# show data in view
tube1Display = Show(stem_tube, renderView1)
tube2Display = Show(root_tube, renderView1)
ribbon1Display = Show(leaf_ribbon, renderView1)
# trace defaults for the display properties.
tube2Display.ColorArrayName = [None, 'organtype']
tube1Display.ColorArrayName = [None, 'organtype']
ribbon1Display.ColorArrayName = [None, 'organtype']
# hide data in view
Hide(cellDatatoPointData1, renderView1)

# set scalar coloring
ColorBy(tube1Display, ('CELLS', 'organtype'))
ColorBy(tube2Display, ('CELLS', 'organtype'))
ColorBy(ribbon1Display, ('CELLS', 'organtype'))

# rescale color and/or opacity maps used to include current data range
tube1Display.RescaleTransferFunctionToDataRange(True)
tube2Display.RescaleTransferFunctionToDataRange(True)
ribbon1Display.RescaleTransferFunctionToDataRange(True)
# show color bar/color legend
tube1Display.SetScalarBarVisibility(renderView1, True)
tube2Display.SetScalarBarVisibility(renderView1, True)
ribbon1Display.SetScalarBarVisibility(renderView1, True)
# get color transfer function/color map for 'type'
typeLUT = GetColorTransferFunction('organtype')

# get opacity transfer function/opacity map for 'type'
typePWF = GetOpacityTransferFunction('organtype')

# Apply a preset using its name. Note this may not work as expected when presets have duplicate names.
typeLUT.ApplyPreset('blot', True)
