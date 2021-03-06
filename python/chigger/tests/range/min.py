#!/usr/bin/env python
#pylint: disable=missing-docstring
#################################################################
#                   DO NOT MODIFY THIS HEADER                   #
#  MOOSE - Multiphysics Object Oriented Simulation Environment  #
#                                                               #
#            (c) 2010 Battelle Energy Alliance, LLC             #
#                      ALL RIGHTS RESERVED                      #
#                                                               #
#           Prepared by Battelle Energy Alliance, LLC           #
#             Under Contract No. DE-AC07-05ID14517              #
#              With the U. S. Department of Energy              #
#                                                               #
#              See COPYRIGHT for full restrictions              #
#################################################################
import chigger
reader = chigger.exodus.ExodusReader('../input/mug_blocks_out.e', timestep=0)
mug = chigger.exodus.ExodusResult(reader, variable='diffused', min=1.5)
cbar = chigger.exodus.ExodusColorBar(mug, primary={'precision':2, 'num_ticks':3, 'notation':'fixed'})
window = chigger.RenderWindow(mug, cbar, size=[300,300], test=True)

for i in range(5):
    reader.setOptions(timestep=i)
    window.write('min_' + str(i) + '.png')
window.start()
