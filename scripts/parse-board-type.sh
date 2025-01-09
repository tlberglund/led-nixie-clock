#
# Some bash to parse CMakeLists.txt, expecting the following line to exist:
#   set(PICO_BOARD pico_w CACHE STRING "Board type")
# or
#   set(PICO_BOARD pico2 CACHE STRING "Board type")
#
# Usage:
#   board_type=$(parse_pico_board CMakeLists.txt)
#

parse_pico_board() {
  local cmake_file=$1
  local pico_board_line
  local pico_board_value
  local board_type=""

  # Check if the file exists
  if [[ ! -f "$cmake_file" ]]; then
    echo "Error: File '$cmake_file' not found."
    return 1
  fi

  # Extract the line with the PICO_BOARD definition
  pico_board_line=$(grep -E '^set\(PICO_BOARD' "$cmake_file")

  if [[ -z "$pico_board_line" ]]; then
    echo "Error: PICO_BOARD not defined in '$cmake_file'."
    return 1
  fi

  # Awww yeah, sed
  pico_board_value=$(echo "$pico_board_line" | sed -n 's/.*set(PICO_BOARD \([^ ]*\) .*/\1/p')

  # Determine the board type
  case "$pico_board_value" in
    pico_w)
      board_type="rp2040"
      ;;
    pico2)
      board_type="rp2350"
      ;;
    *)
      echo "Error: Unknown PICO_BOARD value '$pico_board_value'."
      return 1
      ;;
  esac

  # Echo (return) the board type
  echo "$board_type"
}
