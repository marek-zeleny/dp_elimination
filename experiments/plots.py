import math
from matplotlib import pyplot as plt, ticker

scaling_factor_units_map = {
    0: "us",
    1: "ms",
    2: "s",
    3: "10^3 s",
}


def get_divider(factor: int):
    def divider(val: int, *args):
        return int(val // factor)
    return divider


def get_axes_scaling_factor(ax: plt.Axes) -> tuple[int, str]:
    min_value, max_value = ax.get_ylim()
    extreme = max(abs(min_value), abs(max_value))
    if extreme < 1000:
        exp = 0
    else:
        exp = math.floor(math.log(extreme, 1000))
        if extreme / (1000 ** exp) < 4:
            exp -= 1
    factor = 1000 ** exp
    unit = scaling_factor_units_map[exp]
    return int(factor), unit


def plot_zbdd_size(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(series["ClauseCounts"], "orange", label="clauses")
    ax.plot(series["NodeCounts"], "red", label="nodes")

    ax.set_title("ZBDD size")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("count")
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_heuristic_accuracy(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(series["HeuristicScores"], "blue", label="heuristic score")
    ax.plot(series["ClauseCountDifference"], "orange", label="clause count difference")

    ax.set_title("Heuristic accuracy")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("(expected) # clauses")
    ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_unit_literals(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    series = metrics["series"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(series["UnitLiteralsRemoved"], "red", label="unit literals")

    ax.set_title("Unit literals removed")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel("# literals")
    ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def plot_absorbed_clauses(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    absorbed_removed = metrics["series"]["AbsorbedClausesRemoved"]
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    xticklabels = range(len(absorbed_removed))
    ax.set_title("Removed absorbed clauses")
    ax.set_xlabel("# invocations")
    ax.set_ylabel("# absorbed clauses removed")
    ax.bar(xticklabels, absorbed_removed, color="coral", width=0.4, label="removed clauses")
    ax.set_ylim(bottom=0)

    ax: plt.Axes = ax.twinx()
    axes.append(ax)
    ax.plot(durations["RemoveAbsorbedClauses_Search"], "green", label="search")
    if len(durations["RemoveAbsorbedClauses_Serialize"]) > 0:
        assert len(durations["RemoveAbsorbedClauses_Build"]) > 0
        ax.plot(durations["RemoveAbsorbedClauses_Serialize"], "blue", label="serialize")
        ax.plot(durations["RemoveAbsorbedClauses_Build"], "cyan", label="build")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_incremental_absorbed(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    absorbed_not_added = metrics["series"]["AbsorbedClausesNotAdded"]
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    xticklabels = range(len(absorbed_not_added))
    ax.set_title("Incremental absorbed clause removal")
    ax.set_xlabel("# invocations")
    ax.set_ylabel("# absorbed clauses not added")
    ax.bar(xticklabels, absorbed_not_added, color="coral", width=0.4, label="absorbed clauses")
    ax.set_ylim(bottom=0)

    ax: plt.Axes = ax.twinx()
    axes.append(ax)
    ax.plot(durations["IncrementalAbsorbedRemoval_Search"], "green", label="search")
    if len(durations["IncrementalAbsorbedRemoval_Serialize"]) > 0:
        assert len(durations["IncrementalAbsorbedRemoval_Build"]) > 0
        ax.plot(durations["IncrementalAbsorbedRemoval_Serialize"], "blue", label="serialize")
        ax.plot(durations["IncrementalAbsorbedRemoval_Build"], "cyan", label="build")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    return fig, axes


def plot_elimination_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(durations["EliminateVar_SubsetDecomposition"], "orange", label="subset decomposition")
    ax.plot(durations["EliminateVar_Resolution"], "brown", label="resolution")
    ax.plot(durations["EliminateVar_TautologiesRemoval"], "royalblue", label="tautologies removal")
    ax.plot(durations["EliminateVar_Unification"], "green", label="unification")
    ax.plot(durations["EliminateVar_Total"], "red", label="total")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration of variable elimination")
    ax.set_xlabel("# eliminated variables")
    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left", ncol=2)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.25)
    return fig, axes


def plot_read_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(durations["ReadFormula_AddClause"], "green", label="clause addition")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration of reading input clause")
    ax.set_xlabel("# added clauses")
    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def plot_write_duration(metrics: dict) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []
    durations = metrics["durations"]

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    ax.plot(durations["WriteFormula_PrintClause"], "orange", label="clause writing")
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration of writing output clause")
    ax.set_xlabel("# written clauses")
    ax.set_ylabel(f"duration ({unit})")
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.15)
    return fig, axes


def create_plots(metrics: dict) -> list[tuple[str, plt.Figure]]:
    figures = []

    fig_zbdd_size, _ = plot_zbdd_size(metrics)
    figures.append(("zbdd_size", fig_zbdd_size))

    fig_eliminated, _ = plot_heuristic_accuracy(metrics)
    figures.append(("heuristic", fig_eliminated))

    fig_unit_literals, _ = plot_unit_literals(metrics)
    figures.append(("unit_literals", fig_unit_literals))

    fig_absorbed, _ = plot_absorbed_clauses(metrics)
    figures.append(("absorbed", fig_absorbed))

    fig_incremental_absorbed, _ = plot_incremental_absorbed(metrics)
    figures.append(("incremental_absorbed", fig_incremental_absorbed))

    fig_elimination_duration, _ = plot_elimination_duration(metrics)
    figures.append(("duration_elimination", fig_elimination_duration))

    fig_read_duration, _ = plot_read_duration(metrics)
    figures.append(("duration_read", fig_read_duration))

    fig_write_duration, _ = plot_write_duration(metrics)
    figures.append(("duration_write", fig_write_duration))

    return figures
