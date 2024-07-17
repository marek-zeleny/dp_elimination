import pandas as pd

global_col = "global"


# interface
def extract_setup_summary_data(metrics: dict, data: dict[str, dict[str, tuple[float, float, float]]],
                               setup_key: str, input_key: str):
    if input_key not in data:
        data[input_key] = dict()
    data[input_key][setup_key] = (
        metrics["durations"]["AlgorithmTotal"][0],
        metrics["series"]["ClauseCounts"][-1],
        max(metrics["series"]["ClauseCounts"]),
        metrics["counters"]["FinalVars"],
    )
    general = (
        metrics["series"]["ClauseCounts"][0],
        metrics["counters"]["InitVars"],
        metrics["counters"]["InitVars"] - metrics["counters"]["MinVar"] + 1,
    )
    if global_col not in data[input_key]:
        data[input_key][global_col] = general
    else:
        assert data[input_key][global_col] == general


def create_setup_summary_table(data: dict[str, dict[str, tuple[float, float, float]]], setups: list[str]) -> pd.DataFrame:
    inputs = sorted(data.keys())
    complete_data = {(global_col, v): [] for v in ["size", "vars", "aux_vars"]}
    complete_data |= {(s, v): [] for s in setups for v in ["duration", "end_size", "max_size", "end_vars"]}
    for _, i_data in sorted(data.items()):
        size, vars, aux_vars = i_data.get(global_col, (float("nan"), float("nan"), float("nan")))
        complete_data[(global_col, "size")].append(size)
        complete_data[(global_col, "vars")].append(vars)
        complete_data[(global_col, "aux_vars")].append(aux_vars)
        for s in setups:
            duration, end_size, max_size, end_vars = i_data.get(s, (float("nan"), float("nan"), float("nan"), float("nan")))
            complete_data[(s, "duration")].append(duration)
            complete_data[(s, "end_size")].append(end_size)
            complete_data[(s, "max_size")].append(max_size)
            complete_data[(s, "end_vars")].append(end_vars)
    df = pd.DataFrame(complete_data, index=inputs)
    return df
