def whole_number_to_label(n: int) -> str:
    if n < 1e3:
        return f"{n}"
    elif n < 1e6:
        return f"{int(round(n / 1e3, 2))}K"
    elif n < 1e9:
        return f"{int(round(n / 1e6, 2))}M"
    elif n < 1e12:
        return f"{int(round(n / 1e9, 2))}B"
    else:
        return f"{int(round(n / 1e12, 2))}T"
