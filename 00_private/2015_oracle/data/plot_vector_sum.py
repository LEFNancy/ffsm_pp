#!/usr/bin/env python
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
from output_parser_lib import *

plotVectorChart_inner((9.791,1609),(9.441,5489),(9.760,4645),"Area [M sq.km]", "Volume [M c.m.]","output/charts/vector_sums/vol_area_broadL.png")
plotVectorChart_inner((4.317,926),(4.667,1943),(4.349,1510),"Area [M sq.km]", "Volume [M c.m.]","output/charts/vector_sums/vol_area_con.png")
plotVectorChart_inner((14.108,2535),(14.108,7432),(14.108,6155),"Area [M sq.km]", "Volume [M c.m.]","output/charts/vector_sums/vol_area_tot.png")
