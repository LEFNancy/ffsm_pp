#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

# original names: cs2008.asc
# target name:    pathogen_pp#chal_frax#2008.asc

for filename in os.listdir("."):
  if filename.startswith("cs"):
    os.rename(filename, 'pathogen_pp#chal_frax#'+filename[2:])
    print("renamed   "+filename+"  IN   "+'pathogen_pp#chal_frax#'+filename[2:])
print("ALL DONE!")