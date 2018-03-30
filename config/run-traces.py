import os, os.path
import traces

def parse_arguments():
    from argparse import ArgumentParser

    parser = ArgumentParser('Run gltrace-sim.')
    option = parser.add_argument

    option("--sbatch", action="store_true", default=False,
           help="Enable batch mode")

    option("--pretend", action="store_true", default=False,
           help="Just check which traces need to be run again.")

    option("--cluster", 
           choices=['local', 'your_own_cluster_conf'],
           default='local',
           help="Cluster")

    option("--filter", default=None,
           help="Filter trace files")

    option("-f", "--force", action="store_true", default=False,
           help="Run anyway, even if trace result already exists.")

    option("--no-fast-forward", action="store_true", default=False,
           help="Start from frame 0.")

    option("-o", "--output-dir", default=None,
           help="Output folder")

    return parser.parse_known_args()

args, gem5_args = parse_arguments()

if args.cluster == 'local':
    GLTRACE_SIM_HOME='/home/uart/gltracesim/gltracesim/'
    os.environ['PIN_HOME'] = GLTRACE_SIM_HOME + 'ext/pin-2.14-71313-gcc.4.4.7-linux/'
    os.environ['APITRACE_HOME'] = GLTRACE_SIM_HOME + 'ext/apitrace/build'
    os.environ['LD_LIBRARY_PATH'] = GLTRACE_SIM_HOME + 'ext/mesa/build/linux-x86_64/gallium/targets/libgl-xlib/'
    TRACE_DIR= GLTRACE_SIM_HOME + '../traces/'
elif args.cluster == 'your_own_cluster_conf':
    GLTRACE_SIM_HOME='/path/to/gltracesim'
else:
    pass

os.environ['GLTRACE_SIM_HOME'] = GLTRACE_SIM_HOME
os.environ['PATH']             = os.environ['PATH']
CMD                            = 'run-gltrace-sim.sh'

args.output_dir = os.path.abspath(args.output_dir)

#
if os.path.exists('.display'):
    display_nbr_file = open('.display', 'r+')
else:
    display_nbr_file = open('.display', 'w+')

num_jobs = 0

for t in traces.traces:

  out_dir = os.path.join(args.output_dir, t.name)

  os.environ['OUT_DIR'] = out_dir

  if args.filter and args.filter not in t.name:
      print 'Skipping %s' % (t.name)
      continue

  print 'Simulating trace %s' % (t.name)     

  sbatch = 'sbatch --job-name %s --workdir %s -o %s -e %s' % (
      t.name,
      out_dir,
      os.path.join(out_dir, 'slurm.out'),
      os.path.join(out_dir, 'slurm.out')
  )

  stat_file = os.path.join(out_dir, 'frame_stats.pb')

  if not os.path.exists(stat_file):
      pass
  elif os.path.getsize(stat_file) == 0:
      print 'stats.txt empty, simulating trace'
  elif os.path.getsize(stat_file) > 0:
      if args.force:
          print 'stats.txt exists, re-simulating'
      else:
          print 'stats.txt exists, skipping'
          continue

  if args.pretend:
      continue

  if not os.path.exists(out_dir):
      os.makedirs(out_dir)

  # Try do read display number
  try:
      display_nbr_file.seek(0)
      display_nbr = int(display_nbr_file.next()) + 1
  except:
      display_nbr = 100

  if t.trace:
      os.environ['GLTRACE_SIM_BIN'] = 'gltracesim-analyze'
  else:
      os.environ['GLTRACE_SIM_BIN'] = 'gltracesim-generate'

  os.environ['XVFB_RUN'] = 'xvfb-run'
  os.environ['XVFB_ARGS'] = '-screen 0 1920x1200x24+32 +extension GLX +render'
  os.environ['XVFB_RUN_ARGS'] = '-n %i -a -e %s' % (
      display_nbr, os.path.join(out_dir, 'xvfb.err')
  )

  os.environ['TRACE_FILE'] = os.path.join(TRACE_DIR, t.input)
  if args.no_fast_forward:
      os.environ['F'] = '0'
      os.environ['W'] = '1000000'
  else:
      os.environ['F'] = str(t.fast_forward)
      os.environ['W'] = str(t.fast_forward + t.sim_window)


  os.system('%s %s' % (
      '/bin/bash' if not args.sbatch else sbatch,
      CMD
  )
  )

  # Update global display number
  display_nbr_file.seek(0)
  display_nbr_file.write('%i\n' %  display_nbr)

  num_jobs += 1

print "Submitted %d jobs." % (num_jobs)
