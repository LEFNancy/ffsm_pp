#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 0 - parameters

# input data filenames
# input data expected format
# - scen;parName;country;region;forType;diamClass;year;value;

forIFiles    = []
prodIFiles   = []
carbonIFiles = []

scenarios = {}
years = []
printChartsFlag = False
printTablesFlag = False
printAATablesFlag = False

chartoutdir = 'charts'
tableoutdir = 'tables'
tablesmaster = '00_master_tables'
chartsmaster = '00_master_charts'
charttype = 'pdf'
sep = ';'



countries = {'FRA': [['AlsChaLor','AquPoi','AuvLim','BasHau','BouFra','BrePay','Cen','Cor','IleNorPic','LanPro','Mid','Rho'],'France']}

regions = {'AlsChaLor': 'Alsace ; Champagne Ardenne ; Lorraine',
               'AquPoi':    'Aquitaine ; Poitou Charentes',
               'AuvLim':    'Auvergne ; Limousin',
               'BasHau':    'Basse Normandie ; Haute Normandie',
               'BouFra':    'Bourgogne : Franche Comté',
               'BrePay':    'Bretagne ; pays de la Loire',
               'Cen':       'Centre Val de Loire',
               'Cor':       'Corse',
               'IleNorPic': 'Ile de France ; Nord Pas de Calais ; Picardie',
               'LanPro':    'Languedoc Roussillon ; Provence Alpes Côtes d\'Azur',
               'Mid':       'Midi Pyrénées',
               'Rho':       'Rhône Alpes',}    

# key: var short name
# value: turple with long name, unit and optionally variable to act for ponderation and name of aggregated variable. 20160815: added info if the ponderation variable is specific for the same ft (keywork: 'sameft') or global for all the forest types ('globalft')
# These should be called forVars
forVars = {'hV': ['Harvested volumes', r"$Mm^3$"],
       'vReg': ['Regeneration volumes', r"$Mm^3$"],
       'vol': ['Forest volumes', r"$Mm^3$"],
       'sumExpReturns': ['Sum of expected returns', r"€"],
       #'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns','globalft'], # the script doesn't use sumExpReturns, it computes itself the value using the pounderation variable and put the value in this temporary variable . Then it does compute the total aggregate using this intermediate variable (as it should be).
       'expReturns': ['Expected returns','€/ha'],
       'forArea': ['Forest area','ha'],
       'harvestedArea': ['Harvested area','ha'],
       'regArea': ['Regeneration area','ha'],
       'STOCK_INV': ['Carbon pool in inventoried forest resources', r"$Mt CO_2$"],
       'STOCK_EXTRA': ['Carbon pool in non-inventoried forest resources (branches, roots)', r"$Mt CO_2$"],
       'STOCK_PRODUCTS': ['Carbon pool in forest products', r"$Mt CO_2$"],
       'EM_ENSUB': ['Cumulative emissions from energy substitution', r"$Mt CO_2$"],
       'EM_MATSUB': ['Cumulative emissions from material substitution', r"$Mt CO_2$"],
       'EM_FOROP': ['Cumulative emissions from forest operations', r"$Mt CO_2$"],
           }  
# key: var short name
# value: list with long name, unit, domain (either pp, tp or p) and optionally a variable to act for ponderation
prodVars = {'st': ['Total supply', r"$Mm^3$", 'p'],
            'pl': ['Local price', r"$Mm^3$", 'p', 'st'],
           }
            
spGroups = ['Fut_Feu', 'Fut_Mix', 'Fut_Con', 'Mel_Feu', 'Mel_Mix', 'Tail_Feu',]
  
pProd = ['hardWRoundW','softWRoundW','pulpWFuelW','ashRoundW']
tProd=['fuelW','hardWSawnW','softWSawnW','plyW','pulpW','pannels','ashSawnW','ashPlyW']




#key: human name
#value[0]: list of sp groups
#value[1]: chart line type
#value[2]: chart line width
#value[3]: (optional) alias in the data. If present, the input data will be converted to the name at input time 
spAggregates = {'00_Total': [['Fut_Feu', 'Fut_Mix', 'Fut_Con', 'Mel_Feu', 'Mel_Mix', 'Tail_Feu',],'-',4,''],
                    '01_Broadleaved': [['Fut_Feu', 'Mel_Feu','Tail_Feu'],'--',3,'broadL'],
                    '02_Coniferous': [['Fut_Con'],':',3,'con'],
                    '03_Mixed': [['Fut_Mix','Mel_Mix'],'-.',3,'mixedBC']
                    }

tvalue001  = [63.6567411629, 9.9248432009, 5.8409093097, 4.6040948714, 4.0321429836, 3.7074280213, 3.4994832974, 3.3553873313, 3.2498355416, 3.1692726726, 3.1058065155, 3.0545395894, 3.0122758387, 2.9768427344, 2.9467128835, 2.9207816224, 2.8982305197, 2.8784404727, 2.8609346065, 2.8453397098, 2.831359558, 2.8187560606, 2.8073356838, 2.7969395048, 2.7874358137, 2.7787145333, 2.7706829571, 2.7632624555, 2.7563859037, 2.7499956536, 2.7440419193, 2.738481482, 2.7332766424, 2.7283943671, 2.7238055892, 2.7194846305, 2.7154087215, 2.7115576019, 2.7079131835, 2.7044592674, 2.7011813036, 2.6980661862, 2.6951020792, 2.6922782657, 2.6895850194, 2.6870134922, 2.6845556179, 2.682204027, 2.6799519736, 2.6777932709]    # invt for alpha=0.01
tvalue0001 = [636.6192487687, 31.5990545764, 12.9239786367, 8.6103015814, 6.8688266259, 5.9588161788, 5.4078825209, 5.0413054334, 4.7809125859, 4.5868938587, 4.4369793382, 4.3177912836, 4.2208317277, 4.1404541127, 4.0727651959, 4.0149963272, 3.9651262721, 3.9216458251, 3.8834058526, 3.8495162749, 3.8192771643, 3.7921306717, 3.7676268043, 3.7453986193, 3.7251439497, 3.7066117435, 3.6895917135, 3.6739064007, 3.6594050195, 3.645958635, 3.6334563498, 3.6218022599, 3.6109130077, 3.6007157974, 3.5911467758, 3.5821497015, 3.5736748444, 3.5656780716, 3.5581200813, 3.5509657609, 3.544183643, 3.5377454453, 3.5316256778, 3.5258013065, 3.520251465, 3.5149572055, 3.5099012834, 3.5050679705, 3.5004428914, 3.4960128818] # invt for alpha=0.001




# -----------------------------------------------------------------------------
# global containers, don't touch
idata = {}
odata = {}
x = []
sortedregions = []
products= pProd+tProd
