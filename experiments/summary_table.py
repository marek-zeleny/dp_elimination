import pandas as pd


# data extraction
def get_duration(metrics):
    return metrics["durations"]["AlgorithmTotal"][0]


def get_remaining_vars_ratio(metrics):
    total = metrics["counters"]["InitVars"] - metrics["counters"]["MinVar"] + 1
    remaining = metrics["counters"]["FinalVars"] - metrics["counters"]["MinVar"] + 1
    return remaining / total


def get_relative_growth(metrics):
    initial = metrics["series"]["ClauseCounts"][0]
    final = metrics["series"]["ClauseCounts"][-1]
    return final / initial


# interface
def extract_setup_summary_data(metrics: dict, data: dict[str, dict[str, tuple[float, float, float]]],
                               setup_key: str, input_key: str):
    if input_key not in data:
        data[input_key] = dict()
    data[input_key][setup_key] = (
        get_duration(metrics),
        get_remaining_vars_ratio(metrics),
        get_relative_growth(metrics),
    )


def create_setup_summary_table(data: dict[str, dict[str, tuple[float, float, float]]], setups: list[str]) -> pd.DataFrame:
    inputs = sorted(data.keys())
    complete_data = {(s, v): [] for s in setups for v in ["duration", "vars", "growth"]}
    for _, i_data in sorted(data.items()):
        for s in setups:
            duration, vars, growth = i_data.get(s, (float("nan"), float("nan"), float("nan")))
            complete_data[(s, "duration")].append(duration)
            complete_data[(s, "vars")].append(vars)
            complete_data[(s, "growth")].append(growth)
    df = pd.DataFrame(complete_data, index=inputs)
    return df
