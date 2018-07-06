##########################################################
### Settings and definitions
##########################################################

type Scenario
  name::String
  mr::Float64
  ec::Float64
  ec_var::Float64
  ep::Float64
  ep_var::Float64
  ra::Float64
  ra_var::Float64
  content::String
end

scen_prefixes = ["MR", "EC", "EP", "RA"]
scen_coeffs = [
  [0.1, 0.5, 0.9],
  [0, 0.5, 1],
  [0, 0.5, 1],
  [0, 1.5, 3],
]

function makeScenarios(scen_prefixes, scen_coeffs)
    scenarios = Scenario[]
    for (mr_idx,mr) in enumerate(scen_coeffs[1])
        for (ec_idx,ec) in enumerate(scen_coeffs[2])
            for (ep_idx,ep) in enumerate(scen_coeffs[3])
                for (ra_idx,ra) in enumerate(scen_coeffs[4])
                    ec_var = (ec_idx == 0 || ec_idx == length(scen_coeffs[2]) ) ? 0 :0.3
                    ep_var = (ep_idx == 0 || ep_idx == length(scen_coeffs[3]) ) ? 0 :0.3
                    ra_var = (ra_idx == 0 || ra_idx == length(scen_coeffs[4]) ) ? 0 :0.3*ra #this is a sd not a cv
                    name = "mr$(mr)-ec$(ec)-ep$(ep)-ra$(ra)"
                    content = """
                    name;type;region;comment;value_0;value_1;value_2;value_3;value_4;value_5;value_6;value_7;value_8;value_9;value_10;value_11;value_12;value_13;value_14;value_15;value_16;value_17;value_18;value_19;value_20;value_21;value_22;value_23;value_24;value_25;value_26;value_27;value_28;value_29;value_30;value_31;value_32;value_33;value_34;value_35;value_36;value_37;value_38;value_39;value_40;value_41;value_42;value_43;value_44;value_45;value_46;value_47;value_48;value_49;value_50;value_51;value_52;value_53;value_54;value_55;value_56;value_57;value_58;value_59;value_60;value_61;value_62;value_63;value_64;value_65;value_66;value_67;value_68;value_69;value_70;value_71;value_72;value_73;value_74;value_75;value_76;value_77;value_78;value_79;value_80;value_81;value_82;value_83;value_84;value_85;value_86;value_87;value_88;value_89;value_90;value_91;value_92;value_93;value_94;value_95;value_96;value_97;value_98;value_99;value_100;value_101;value_102;value_103;value_104;value_105
                    #;;;;2005;2006;2007;2008;2009;2010;2011;2012;2013;2014;2015;2016;2017;2018;2019;2020;2021;2022;2023;2024;2025;2026;2027;2028;2029;2030;2031;2032;2033;2034;2035;2036;2037;2038;2039;2040;2041;2042;2043;2044;2045;2046;2047;2048;2049;2050;2051;2052;2053;2054;2055;2056;2057;2058;2059;2060;2061;2062;2063;2064;2065;2066;2067;2068;2069;2070;2071;2072;2073;2074;2075;2076;2077;2078;2079;2080;2081;2082;2083;2084;2085;2086;2087;2088;2089;2090;2091;2092;2093;2094;2095;2096;2097;2098;2099;2100;2101;2102;2103;2104;2105;2106;2107;2108;2109;2110
                    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    mr;double;;;$(mr);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    expType;double;;;$(ec);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    expType_cv;double;;;$(ec_var);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    expTypePrices;double;;;$(ep);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    expTypePrices_cv;double;;;$(ep_var);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    riskAversionAgentAverage;double;;;$(ra);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    riskAversionAgentSd;double;;;$(ra_var);;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                    """
                    scen = Scenario(name, mr,ec,ec_var,ep,ep_var,ra,ra_var,content)
                    push!(scenarios,scen)
                end
            end
        end
    end
    return scenarios
end


builtScenarios = makeScenarios(scen_prefixes, scen_coeffs)


scenarioIdsTable = ""
for s in builtScenarios
  scenarioIdsTable = scenarioIdsTable * "$(s.name)\n"
end
open("scenarioIdsTable.txt", "w") do f
    write(f, scenarioIdsTable)
end


scenarioDetailsTable = "id\tmr\texpType\texpType_cv\texpTypePrices\texpTypePrices_cv\triskAversionAgentAverage\triskAversionAgentSd\n"
for s in builtScenarios
  scenarioDetailsTable = scenarioDetailsTable * "$(s.name)\t$(s.mr)\t$(s.ec)\t$(s.ec_var)\t$(s.ep)\t$(s.ep_var)\t$(s.ra)\t$(s.ra_var)\n"
end
open("scenarioDetailsTable.csv", "w") do f
    write(f, scenarioDetailsTable)
end

scenarioFilenamesTable = ""
for s in builtScenarios
  scenarioFilenamesTable = scenarioFilenamesTable * "$(s.name)\t\t\t\t\t\t\t\tscenarios/$(s.name).csv\n"
end
open("scenarioFilenamesTable.csv", "w") do f
    write(f, scenarioFilenamesTable)
end

runScenarios ="""
#!/bin/bash\n
#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------\n\n
# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_2018_mgm2cbl.sh\n
# Management to carbon balance scenarios
"""
for s in builtScenarios
  runScenarios = runScenarios * "  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' '$(s.name)'\n"
end
open("runscenarios_2018_mgm2cbl.sh", "w") do f
    write(f, runScenarios)
end

scenariosBaseDir = "scenarios/"
for s in builtScenarios
  open("$(scenariosBaseDir)$(s.name).csv", "w") do f
      write(f, runScenarios)
  end
end
