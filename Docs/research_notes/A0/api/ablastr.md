# A0 API catalog — ablastr

> Generated from headers; 17 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `PoissonInterpCPtoFP`

- file: `Source/ablastr/fields/Interpolate.H`
- kind: `struct`
- public methods: 1; public members: 3; private members: 0

```cpp
PoissonInterpCPtoFP( amrex::Array4<amrex::Real> const phi_fp_arr, amrex::Array4<amrex::Real const> const phi_cp_arr, amrex::IntVect const refratio) : m_phi_fp_arr(phi_fp_arr), m_phi_cp_arr(phi_cp_arr), m_refratio(refratio)
```

Public members:

```cpp
amrex::Array4<amrex::Real> m_phi_fp_arr
amrex::Array4<amrex::Real const> m_phi_cp_arr
amrex::IntVect m_refratio
```

## `MLMGOptions`

- file: `Source/ablastr/fields/MLMGOptions.H`
- kind: `struct`
- public methods: 4; public members: 12; private members: 0

```cpp
amrex::Real relative_tolerance = amrex::Real(1.e-11)
amrex::Real absolute_tolerance = amrex::Real(0.0)
amrex::Real bottom_relative_tolerance = amrex::Real(-1.0)
amrex::Real bottom_absolute_tolerance = amrex::Real(-1.0)
```

Public members:

```cpp
int max_iters = 200
int verbosity = 2
int num_final_sweeps = 8
amrex::BottomSolver bottom_solver = amrex::BottomSolver::Default
int bottom_verbosity = 0
int bottom_max_iters = -1
int max_coarsening_level = -1
int agglomeration = -1
int agglomeration_grid_size = -1
int consolidation = -1
int consolidation_grid_size = -1
std::string hypre_options_namespace = "hypre"
```

## `Direction`

- file: `Source/ablastr/fields/MultiFabRegister.H`
- kind: `class`
- public methods: 2; public members: 12; private members: 1

```cpp
constexpr explicit Direction (int d) : dir(d)
bool operator<(const Direction& other) const { return this->dir < other.dir; } operator std::string() const { if (dir == r) { return std::string{r_string}; } if (dir == theta) { return std::string{theta_string}; } if (dir == phi) { return std::string{phi_string}; } if (dir == x) { return std::string{x_string}; } if (dir == y) { return std::string{y_string}; } if (dir == z) { return std::string{z_string}; } throw std::runtime_error("invalid direction: " + std::to_string(dir)); return std::to_string(dir); } constexpr Direction (const std::string_view sv) : dir{-1} { if (sv == r_string) { dir = r; } if (sv == theta_string) { dir = theta; } if (sv == phi_string) { dir = phi; } if (sv == x_string) { dir = x; } if (sv == y_string) { dir = y; } if (sv == z_string) { dir = z; } if (dir == -1) { throw std::runtime_error("invalid direction: " + std::string{sv}); } } Direction (std::string const & s) : Direction{std::string_view{s}} {} constexpr Direction (char const * c) : Direction{checked_string_view(c)} {} Direction (char) = delete; Direction (std::nullptr_t) = delete; constexpr operator int() const { return dir; }
```

Public members:

```cpp
static const Direction r
static const Direction theta
static constexpr std::string_view r_string = "r"
static constexpr std::string_view theta_string = "theta"
static const Direction phi
static constexpr std::string_view phi_string = "phi"
static const Direction x
static const Direction y
static constexpr std::string_view x_string = "x"
static constexpr std::string_view y_string = "y"
static const Direction z
static constexpr std::string_view z_string = "z"
```

## `MultiFabOwner`

- file: `Source/ablastr/fields/MultiFabRegister.H`
- kind: `struct`
- public methods: 0; public members: 7; private members: 0

Public members:

```cpp
amrex::MultiFab m_mf
std::optional<Direction> m_dir = std::nullopt
int m_level = 0
bool m_remake = true
bool m_redistribute_on_remake = true
bool m_checkpoint_restart = false
std::string m_owner
```

## `MultiFabRegister`

- file: `Source/ablastr/fields/MultiFabRegister.H`
- kind: `struct`
- public methods: 34; public members: 1; private members: 1

```cpp
MultiFabRegister() = default
MultiFabRegister(MultiFabRegister const &) = delete
MultiFabRegister(MultiFabRegister&&) = delete
MultiFabRegister& operator=(MultiFabRegister const &) = delete
MultiFabRegister& operator=(MultiFabRegister&&) = delete
~MultiFabRegister() = default
amrex::MultiFab* alloc_init ( T name, int level, amrex::BoxArray const & ba, amrex::DistributionMapping const & dm, int ncomp, amrex::IntVect const & ngrow, std::optional<amrex::Real const> initial_value = std::nullopt, bool remake = true, bool redistribute_on_remake = true, bool checkpoint_restart = false )
amrex::MultiFab* alloc_init ( T name, Direction dir, int level, amrex::BoxArray const & ba, amrex::DistributionMapping const & dm, int ncomp, amrex::IntVect const & ngrow, std::optional<amrex::Real const> initial_value = std::nullopt, bool remake = true, bool redistribute_on_remake = true, bool checkpoint_restart = false )
amrex::MultiFab* alias_init ( N new_name, A alias_name, int level, std::optional<amrex::Real const> initial_value = std::nullopt )
amrex::MultiFab* alias_init ( N new_name, A alias_name, Direction dir, int level, std::optional<amrex::Real const> initial_value = std::nullopt )
[[nodiscard]] bool has ( T name, int level ) const
[[nodiscard]] bool has ( T name, Direction dir, int level ) const
[[nodiscard]] bool has_vector ( T name, int level ) const
[[nodiscard]] amrex::MultiFab* get ( T name, int level )
[[nodiscard]] amrex::MultiFab* get ( T name, Direction dir, int level )
[[nodiscard]] amrex::MultiFab const * get ( T name, int level ) const
[[nodiscard]] amrex::MultiFab const * get ( T name, Direction dir, int level ) const
[[nodiscard]] MultiLevelScalarField get_mr_levels ( T name, int finest_level, bool skip_level_0=false )
[[nodiscard]] ConstMultiLevelScalarField get_mr_levels ( T name, int finest_level, bool skip_level_0=false ) const
[[nodiscard]] VectorField get_alldirs ( T name, int level )
[[nodiscard]] ConstVectorField get_alldirs ( T name, int level ) const
[[nodiscard]] MultiLevelVectorField get_mr_levels_alldirs ( T name, int finest_level, bool skip_level_0=false )
[[nodiscard]] ConstMultiLevelVectorField get_mr_levels_alldirs ( T name, int finest_level, bool skip_level_0=false ) const
[[nodiscard]] std::vector<std::string> list () const
void erase ( T name, int level )
void erase ( T name, Direction dir, int level )
void clear_level ( int level )
void remake_level ( int other_level, amrex::DistributionMapping const & new_dm )
void write_checkpoints ( int level, std::string const & dir )
void read_restarts ( int level, std::string const & dir )
[[nodiscard]] std::string mf_name ( std::string name, int level ) const
[[nodiscard]] std::string mf_name ( std::string name, Direction dir, int level ) const
[[nodiscard]] bool internal_has ( std::string const & internal_name )
[[nodiscard]] amrex::MultiFab * internal_get ( std::string const & internal_name )
```

Public members:

```cpp
static inline std::vector<Direction> m_all_dirs =
```

## `is_castable_to_string`

- file: `Source/ablastr/fields/MultiFabRegister.H`
- kind: `struct`, bases: `std::false_type`
- public methods: 0; public members: 0; private members: 0

## `FFTplan`

- file: `Source/ablastr/math/fft/AnyFFT.H`
- kind: `struct`
- public methods: 0; public members: 6; private members: 0

Public members:

```cpp
amrex::Real* m_real_array
Complex* m_complex_array
VendorFFTPlan m_plan
direction m_dir
int m_dim
amrex::gpuStream_t m_stream
```

## `KernelTimer`

- file: `Source/ablastr/parallelization/KernelTimer.H`
- kind: `class`
- public methods: 5; public members: 0; private members: 3

```cpp
~KernelTimer () = default
KernelTimer ( KernelTimer const &) = default
KernelTimer& operator= ( KernelTimer const & ) = default
KernelTimer ( KernelTimer&& ) = default
KernelTimer& operator= ( KernelTimer&& ) = default
```

## `SignalHandling`

- file: `Source/ablastr/utils/SignalHandling.H`
- kind: `class`
- public methods: 6; public members: 3; private members: 4

```cpp
static int parseSignalNameToNumber (const std::string &str)
static void InitSignalHandling ()
static void CheckSignals ()
static void WaitSignals ()
static bool TestAndResetActionRequestFlag (int action_to_test)
SignalHandling () = delete
```

Public members:

```cpp
static constexpr int NUM_SIGNALS = 32
enum signal_action_requested_labels
static bool signal_conf_requests[SIGNAL_REQUESTS_SIZE][NUM_SIGNALS]
```

## `Logger`

- file: `Source/ablastr/utils/msg_logger/MsgLogger.H`
- kind: `class`
- public methods: 6; public members: 0; private members: 4

```cpp
Logger()
void record_msg(const Msg& msg)
[[nodiscard]] std::vector<Msg> get_msgs() const
[[nodiscard]] std::vector<MsgWithCounter> get_msgs_with_counter() const
[[nodiscard]] std::vector<MsgWithCounterAndRanks> collective_gather_msgs_with_counter_and_ranks() const
void clear()
```

## `Msg`

- file: `Source/ablastr/utils/msg_logger/MsgLogger.H`
- kind: `struct`
- public methods: 3; public members: 3; private members: 0

```cpp
[[nodiscard]] std::vector<char> serialize() const
static Msg deserialize(std::vector<char>::const_iterator& it)
static Msg deserialize(std::vector<char>::const_iterator&& rit)
```

Public members:

```cpp
std::string topic
std::string text
Priority priority
```

## `MsgWithCounter`

- file: `Source/ablastr/utils/msg_logger/MsgLogger.H`
- kind: `struct`
- public methods: 3; public members: 2; private members: 0

```cpp
[[nodiscard]] std::vector<char> serialize() const
static MsgWithCounter deserialize(std::vector<char>::const_iterator& it)
static MsgWithCounter deserialize(std::vector<char>::const_iterator&& rit)
```

Public members:

```cpp
Msg msg
std::int64_t counter
```

## `MsgWithCounterAndRanks`

- file: `Source/ablastr/utils/msg_logger/MsgLogger.H`
- kind: `struct`
- public methods: 3; public members: 3; private members: 0

```cpp
[[nodiscard]] std::vector<char> serialize() const
static MsgWithCounterAndRanks deserialize(std::vector<char>::const_iterator& it)
static MsgWithCounterAndRanks deserialize(std::vector<char>::const_iterator&& rit)
```

Public members:

```cpp
MsgWithCounter msg_with_counter
bool all_ranks
std::vector<int> ranks
```

## `IntervalsParser`

- file: `Source/ablastr/utils/text/IntervalsParser.H`
- kind: `class`
- public methods: 8; public members: 0; private members: 3

```cpp
IntervalsParser () = default
explicit IntervalsParser (const std::vector<std::string>& instr_vec)
[[nodiscard]] bool contains (int n) const
[[nodiscard]] int nextContains (int n) const
[[nodiscard]] int previousContains (int n) const
[[nodiscard]] int previousContainsInclusive (int n) const
[[nodiscard]] int localPeriod (int n) const
[[nodiscard]] bool isActivated () const
```

## `SliceParser`

- file: `Source/ablastr/utils/text/IntervalsParser.H`
- kind: `class`
- public methods: 8; public members: 0; private members: 4

```cpp
explicit SliceParser (const std::string& instr, bool require_stop=false)
[[nodiscard]] bool contains (int n) const
[[nodiscard]] int nextContains (int n) const
[[nodiscard]] int previousContains (int n) const
[[nodiscard]] int getPeriod () const
[[nodiscard]] int getStart () const
[[nodiscard]] int getStop () const
[[nodiscard]] int numContained() const
```

## `Timer`

- file: `Source/ablastr/utils/timer/Timer.H`
- kind: `class`
- public methods: 5; public members: 0; private members: 2

```cpp
Timer() = default
void record_start_time() noexcept
void record_stop_time() noexcept
[[nodiscard]] double get_duration () const noexcept
[[nodiscard]] double get_global_duration () const
```

## `WarnManager`

- file: `Source/ablastr/warn_manager/WarnManager.H`
- kind: `class`
- public methods: 15; public members: 2; private members: 4

```cpp
WarnManager( const WarnManager& ) = delete
WarnManager( WarnManager&& ) = delete
WarnManager& operator=( const WarnManager & ) = delete
WarnManager& operator=( const WarnManager&& ) = delete
~WarnManager() = default
static WarnManager& GetInstance()
void RecordWarning( const std::string& topic, const std::string& text, const WarnPriority& priority = WarnPriority::medium)
[[nodiscard]] std::string PrintLocalWarnings( const std::string& when) const
[[nodiscard]] std::string PrintGlobalWarnings( const std::string& when) const
void SetAlwaysWarnImmediately(bool always_warn_immediately)
[[nodiscard]] bool GetAlwaysWarnImmediatelyFlag() const
void SetAbortThreshold(std::optional<WarnPriority> abort_threshold)
[[nodiscard]] std::optional<WarnPriority> GetAbortThreshold() const
void debug_read_warnings_from_input(const amrex::ParmParse& params)
void Clear()
```

Public members:

```cpp
static const int warn_line_size = 80
static const int warn_tab_size = 5
```

