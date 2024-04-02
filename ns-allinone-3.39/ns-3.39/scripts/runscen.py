
import os
import time 


from dataclasses import dataclass
import numpy as np
import logging
import subprocess
import json




ns3Path = '/ns-allinone-3.39/ns-3.39'
ns3Scenario = 'p2p_scenarioReusable_taps'
ns3Scenarioconf = 'scratch/scen.json'
ns3Path = '/ns-allinone-3.39/ns-3.39'

band = 20

import numpy as np

# Type_of_combis = ["CU-plane", "C-plane"]
Type_of_combis = ["CU-plane"]

Weights = {0:"73 9 18"}
# Weights = {0:"23 72 5",1:"25 70 5"} # Mod weights

# Weights = {0:"23 73 4",1:"17 77 6"} # Original Weights - not sim with that _DONE IT_
Marking_Port = {0:"8080 46 9090 46 10800 8 10900 16 11000 24"}

# Marking_Port = {0:"8080 46 9090 46 10800 8 10900 16 11000 24", 1:"8080 8 9090 46 10800 8 10900 16 11000 24"}

# Link Capacity Swept Configuration
Mode = "DL_UPandLowSep"
Start = 55.14
Stop = 55.3
Step = 0.04
Cap_swept = np.arange(Start, Stop+Step, Step, dtype=float)  


def main():
    for index, combi_name in enumerate(Type_of_combis):
        # index = index + 1 
        print(f"{combi_name} - {index}")
        print(f"Weights: {Weights[index]}")
        print(f"Marking Port: {Marking_Port[index]}")
        for cap in Cap_swept:
            filename = f"MOD_{Mode}_{combi_name}_{band}M_{cap:.2f}"
            # Change number of streams (json file)
            cmd = f"mkdir -p ./sim_results/{filename}"  # Use -p to avoid 'File exists' error
            os.system(cmd)
            with open(f"./scratch/scen_{band}_{Mode}.json", "r") as jsonFileReceiver:
                data = json.load(jsonFileReceiver)
            data["FolderName"] = f"{filename}"
            print(f"Link capacity: {cap:.2f}")
            data["MidLinkCap"] = cap  # Convert cap to int here
            
            # data["Weights"] = Weights[index]
            # data["Marking_Port"] = Marking_Port[index]
            with open("./scratch/scen_ex.json", "w") as jsonFileReceiver:
                json.dump(data, jsonFileReceiver)

            cmd = "./ns3 run scratch/juniper-setupnewlinks.cc" 
            os.system(cmd)    

            os.system(f"tar -C ./sim_results -zcvf ./sim_results/{filename}.tar {filename}")
            # os.system(f"rm -r sim_results/{filename}/")

if __name__ == '__main__':
    main()
