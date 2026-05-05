The translated code is now a single Python file—no Makefile or headers are required anymore.

### Usage
The call remains identical to the original:
```bash    
python flood_hydrograph.py -s 1.6 -q 177.9 -t 3 -i discharge.csv -o out.csv
```

### Dependencies
If they are not already installed, you will need:
```bash
pip install numpy pandas
```

---

### Key Translation Decisions

* **Pointer-Arrays → NumPy:** All `double *`, `int *`, and `malloc` constructs have been replaced by `np.ndarray`. For example, `Gradient_discharge` is now resolved using `np.empty()` and NumPy slicing, which is significantly shorter and faster.
* **Structs → pandas DataFrame:** `ST_DATA`, with its fields `y`, `m`, `d`, `h`, and `Qsim`, is now read directly into a `pd.DataFrame`. This also makes writing the output much easier.
* **getopt → argparse:** Command line processing is now handled by Python’s standard `argparse` library.
* **Gradient_percentile → np.percentile:** The $O(n^2)$ Bubble Sort implementation from the original C code has been replaced by NumPy’s built-in percentile function.
* **Line counting eliminated:** In the C code, the file had to be read twice—first to count the lines and then to read them. `pd.read_csv` handles this automatically in one pass.
