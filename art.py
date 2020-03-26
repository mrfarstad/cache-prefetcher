from matplotlib import pyplot as plt

stop_words = [
    "sequential_on_miss",
    "sequential_on_access",
    "adaptive_sequential",
    "user",
    "rpt",
    "dcpt-p",
    "dcpt",
    "tagged",
    "none",
]

change = [
    "seq_on_miss",
    "seq_on_access",
    "adaptive_seq",
    "G/DC",
    "RPT",
    "DCPT-P",
    "DCPT",
]

user_results = {}
with open("width_degree_3/ghb_ait_256/stats.txt", "r") as file:
    lines = file.readlines()
    for i in range(0, len(lines)):
        if "TEST: art110" in lines[i]:
            # print(lines[i:])
            user_stats = lines[i:]
            break

    for line in user_stats:
        if "TEST: art470" in line:
            break
        for word in stop_words:
            if word in line:
                test_results = line.split()
                try:
                    test_results[0] = change[stop_words[:7].index(
                        test_results[0])]
                except Exception:
                    print("")
                user_results[test_results[0]] = float(test_results[2])
                print(test_results[0], user_results[test_results[0]])

plt.ylim([0.93, 1.07])
plt.ylabel("Speedup")
plt.bar(list(reversed(list(user_results.keys()))),
        list(reversed(list(user_results.values()))))
plt.yticks(list(user_results.values()))
plt.xticks(rotation='vertical')
plt.tight_layout()
plt.savefig("art")
plt.show()
