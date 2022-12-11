import subprocess
import os
import json
import re

runs = []


def run(N):

    runs = []

    os.system(f"../pvcSeq {N} > outSeq")
    with open("outSeq", 'r') as f:
        output = f.read()
        timeSeq = re.search(
            r'Tempo de resposta sem considerar E\/S, em segundos: (\d+\.\d+)s', output).group(1)
        timeSeq = float(timeSeq)

    os.system('sed -i "$ d" outSeq')

    for t in range(2, 5):
        os.system(f"mpirun -np {t} ../pvcPar {N} > outPar")
        with open("outPar", 'r') as f:
            output = f.read()
            timePar = re.search(
                r'Tempo de resposta sem considerar E\/S, em segundos: (\d+\.\d+)s', output).group(1)
            timePar = float(timePar)

        os.system('sed -i "$ d" outPar')

        diffCode = subprocess.call(["diff", "outSeq", "outPar"])

        runs.append({
            "N": N,
            "T": t,
            "timeSeq": timeSeq,
            "timePar": timePar,
            "error": diffCode != 0
        })

    os.unlink("outSeq")
    os.unlink("outPar")

    return runs


runs = []

for N in range(2, 14):
    runs += run(N)


with open("runs.json", 'w') as f:
    f.write(json.dumps(runs, indent=4))
