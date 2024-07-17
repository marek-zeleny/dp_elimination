import pandas as pd

global_col_name = "global"


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
    if global_col_name not in data[input_key]:
        data[input_key][global_col_name] = general
    else:
        assert data[input_key][global_col_name] == general


def create_setup_summary_table(data: dict[str, dict[str, tuple[float, float, float]]], setups: list[str]) -> pd.DataFrame:
    inputs = sorted(data.keys())
    global_cols = ["size", "vars", "aux_vars"]
    local_cols = ["duration", "end_size", "max_size", "end_vars"]
    complete_data = {(global_col_name, v): [] for v in global_cols}
    complete_data |= {(s, v): [] for s in setups for v in local_cols}
    for _, i_data in sorted(data.items()):
        values = i_data.get(global_col_name, tuple([float("nan")] * len(global_cols)))
        for c, v in zip(global_cols, values):
            complete_data[(global_col_name, c)].append(v)
        for s in setups:
            values = i_data.get(s, tuple([float("nan")] * len(local_cols)))
            for c, v in zip(local_cols, values):
                complete_data[(s, c)].append(v)
    df = pd.DataFrame(complete_data, index=inputs)
    return df
