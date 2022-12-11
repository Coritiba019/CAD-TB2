import json

runs = json.load(open("../ra.json"))

infos = []

for run in runs:

    if(run["error"]):
        print("FOUND ERROR")
        print(run)

    speedup = run["timeSeq"] / run["timePar"]
    efficiency = speedup / run["T"]
    info = {
        "N": run["N"],
        "T": run["T"],
        "Speedup": f'{speedup:.2f}',
        "Efficiency": f'{efficiency:.2f}',
        "Sequential Time": f'{run["timeSeq"]:.6f}s',
        "Parallel Time": f'{run["timePar"]:.6f}s',
    }
    infos.append(info)

json.dump(infos, open("data.json", "w"), indent=4)
