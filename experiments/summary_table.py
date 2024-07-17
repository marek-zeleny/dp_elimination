import pandas as pd


# interface
def extract_setup_summary_data(metrics: dict, data: dict[str, dict[str, tuple[float, float, float]]],
                               setup_key: str, input_key: str):
    if input_key not in data:
        data[input_key] = dict()
    data[input_key][setup_key] = (
        metrics["durations"]["AlgorithmTotal"][0],
        metrics["series"]["ClauseCounts"][-1],
        metrics["counters"]["FinalVars"],
    )
    general = (
        metrics["series"]["ClauseCounts"][0],
        metrics["counters"]["InitVars"],
        metrics["counters"]["InitVars"] - metrics["counters"]["MinVar"] + 1,
    )
    if "general" not in data[input_key]:
        data[input_key]["general"] = general
    else:
        assert data[input_key]["general"] == general


def create_setup_summary_table(data: dict[str, dict[str, tuple[float, float, float]]], setups: list[str]) -> pd.DataFrame:
    inputs = sorted(data.keys())
    complete_data = {(v, ""): [] for v in ["size", "vars", "aux_vars"]}
    complete_data |= {(s, v): [] for s in setups for v in ["duration", "end_size", "end_vars"]}
    for _, i_data in sorted(data.items()):
        size, vars, aux_vars = i_data.get("general", (float("nan"), float("nan"), float("nan")))
        complete_data[("size", "")].append(size)
        complete_data[("vars", "")].append(vars)
        complete_data[("aux_vars", "")].append(aux_vars)
        for s in setups:
            duration, end_size, end_vars = i_data.get(s, (float("nan"), float("nan"), float("nan")))
            complete_data[(s, "duration")].append(duration)
            complete_data[(s, "end_size")].append(end_size)
            complete_data[(s, "end_vars")].append(end_vars)
    df = pd.DataFrame(complete_data, index=inputs)
    return df
