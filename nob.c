#define NOB_IMPLEMENTATION
#include "src/nob.h"

const char *linux_compiler_c = "cc";
const char *linux_compiler_cxx = "c++";
const char *windows_compiler_c = "x86_64-w64-mingw32-cc";
const char *windows_compiler_cxx = "x86_64-w64-mingw32-c++";

const char *linux_linker = "ar";
const char *windows_linker = "x86_64-w64-mingw32-ar";

typedef enum { PLATFORM_LINUX, PLATFORM_WINDOWS } build_platform_t;

bool read_dir_recursively(const char *parent, Nob_File_Paths *children) {
  bool result = true;
  Nob_File_Paths temp = {0};

  if (nob_get_file_type(parent) != NOB_FILE_DIRECTORY)
    nob_return_defer(result);

  if (!nob_read_entire_dir(parent, &temp))
    nob_return_defer(false);

  for (size_t i = 0; i < temp.count; ++i) {
    const char *full_path = nob_temp_sprintf("%s/%s", parent, temp.items[i]);

    Nob_File_Type type = nob_get_file_type(full_path);
    if (type < 0)
      nob_return_defer(false);

    switch (type) {
    case NOB_FILE_DIRECTORY: {
      if (strcmp(temp.items[i], ".") == 0)
        continue;
      if (strcmp(temp.items[i], "..") == 0)
        continue;

      if (!read_dir_recursively(full_path, children))
        nob_return_defer(false);
    } break;

    case NOB_FILE_REGULAR: {
      nob_da_append(children, full_path);
    } break;

    case NOB_FILE_SYMLINK: {
    } break;

    case NOB_FILE_OTHER: {
      nob_log(NOB_ERROR, "Unsupported type of file %s", full_path);
      nob_return_defer(false);
    } break;

    default:
      NOB_ASSERT(0 && "unreachable");
    }
  }

defer:
  nob_da_free(temp);
  return result;
}

static void usage(const char *program) {
  printf("%s [--windows | --linux] <-r> [args]\n", program);
  printf("\t--release: Tries to compile with optimizations and without debug "
         "symbols\n");
  printf("\t--linux: Tries to compile for linux with gcc\n");
  printf("\t--windows: Tries to compile for windows with mingw\n");
  printf("\t-r: Tries to run the executable immediately after "
         "building, it passes everything that comes after it to the "
         "executable as arguments\n");
}

static char *get_file_extension(const char *fileName) {
  char *dot = strrchr(fileName, '.');
  if (!dot || dot == fileName)
    return NULL;
  return dot;
}

static void strip_first_dir(Nob_File_Paths *paths) {
  for (size_t i = 0; i < paths->count; ++i)
    paths->items[i] = strchr(paths->items[i], '/') + 1;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  const char *program = nob_shift_args(&argc, &argv);

  bool release = false;
#ifdef _WIN32
  build_platform_t platform = PLATFORM_WINDOWS;
#else
  build_platform_t platform = PLATFORM_LINUX;
#endif // _WIN32
  bool run_flag = false;

  while (argc > 0) {
    const char *subcmd = nob_shift_args(&argc, &argv);
    if (strcmp(subcmd, "--release") == 0)
      release = true;
    else if (strcmp(subcmd, "--linux") == 0)
      platform = PLATFORM_LINUX;
    else if (strcmp(subcmd, "--windows") == 0)
      platform = PLATFORM_WINDOWS;
    else if (strcmp(subcmd, "-r") == 0) {
      run_flag = true;
      break;
    } else {
      nob_log(NOB_ERROR, "Unknown flag %s", subcmd);
      usage(program);
      return 1;
    }
  }

  if (!nob_mkdir_if_not_exists("build"))
    return false;

  const char *release_string;
  if (release)
    release_string = "release";
  else
    release_string = "debug";

  const char *compiler;
  if (platform == PLATFORM_LINUX)
    compiler = linux_compiler_c;
  else if (platform == PLATFORM_WINDOWS)
    compiler = windows_compiler_c;

  const char *platform_string;
  if (platform == PLATFORM_LINUX)
    platform_string = "linux";
  else if (platform == PLATFORM_WINDOWS)
    platform_string = "windows";

  const char *platform_build_path =
      nob_temp_sprintf("build/%s", platform_string);

  if (!nob_mkdir_if_not_exists(platform_build_path))
    return 1;

  const char *build_path =
      nob_temp_sprintf("%s/%s", platform_build_path, release_string);

  if (!nob_mkdir_if_not_exists(build_path))
    return 1;
  
  const char *exe;
  if (platform == PLATFORM_LINUX)
    exe = nob_temp_sprintf("%s/bavis", build_path);
  else if (platform == PLATFORM_WINDOWS)
    exe = nob_temp_sprintf("%s/bavis.exe", build_path);

  Nob_Cmd cmd = {0};
  Nob_File_Paths input_files = {0};
  Nob_File_Paths object_files = {0};
  Nob_Procs procs = {0};
  Nob_File_Paths inputs = {0};

  if (!read_dir_recursively("src", &inputs))
    return 1;

  strip_first_dir(&inputs);

  for (size_t i = 0; i < inputs.count; ++i) {
    if (strcmp(get_file_extension(inputs.items[i]), ".c") != 0)
      continue;

    const char *input_path = nob_temp_sprintf("src/%s", inputs.items[i]);
    nob_da_append(&input_files, nob_temp_strdup(input_path));
    char *temp_path = nob_temp_strdup(inputs.items[i]);
    get_file_extension(temp_path)[1] = 'o';
    get_file_extension(temp_path)[2] = '\0';
    const char *output_path = nob_temp_sprintf("%s/%s", build_path, temp_path);
    nob_da_append(&object_files, output_path);

    if (nob_needs_rebuild(output_path, &input_path, 1)) {
      cmd.count = 0;
      nob_cmd_append(&cmd, compiler);
      nob_cmd_append(&cmd, "-Wall", "-Wextra");
      if (release)
        nob_cmd_append(&cmd, "-O2", "-s");
      else
        nob_cmd_append(&cmd, "-O0", "-ggdb");
      nob_cmd_append(&cmd, "-c", input_path);
      nob_cmd_append(&cmd, "-o", output_path);
      Nob_Proc proc = nob_cmd_run_async(cmd);
      nob_da_append(&procs, proc);
    }
  }

  if (!nob_procs_wait(procs))
    return 1;

  cmd.count = 0;
  if (nob_needs_rebuild(exe, object_files.items, object_files.count)) {
    nob_cmd_append(&cmd, compiler);
    nob_cmd_append(&cmd, "-Wall", "-Wextra");
    if (release)
      nob_cmd_append(&cmd, "-O2", "-s");
    else
      nob_cmd_append(&cmd, "-O0", "-ggdb");
    nob_cmd_append(&cmd, "-o", exe);
    nob_da_append_many(&cmd, object_files.items, object_files.count);
    if (platform == PLATFORM_WINDOWS) {
      nob_cmd_append(&cmd, "-static");
    }
    if (!nob_cmd_run_sync(cmd))
      return 1;
  } else {
    nob_log(NOB_INFO, "Executable is already up to date");
  }

  if (run_flag) {
    cmd.count = 0;
    nob_cmd_append(&cmd, exe);
    nob_da_append_many(&cmd, argv, argc);
    if (!nob_cmd_run_sync(cmd))
      return 1;
  }

  return 0;
}
