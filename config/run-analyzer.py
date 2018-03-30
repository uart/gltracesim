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

    option("--filter", 
           default=None,
           help="Filter trace files")

    option("-f", "--force", 
           action="store_true", 
           default=False,
           help="Run anyway, even if trace result already exists.")

    option("--no-fast-forward", 
           action="store_true", 
           default=False,
           help="Start from frame 0.")

    option("--no-sim-end", 
           action="store_true", 
           default=False,
           help="Stop after specified # frames.")

    option("-n", "--num-gpu-cores", 
           type=int,
           default=1,
           help="Number of GPU cores.")

    option("-m", "--models-file", 
           help="Input models files.")

    option("-i", "--input-dir", 
           default=None,
           help="Input folder")

    option("-o", "--output-dir", 
           default=None,
           help="Output folder")

    return parser.parse_known_args()

args, extra_args = parse_arguments()

if args.cluster == 'local':
    GLTRACE_SIM_HOME='/home/uart/gltracesim/gltracesim/'
elif args.cluster == 'your_own_cluster_conf':
    GLTRACE_SIM_HOME='/path/to/gltracesim/'
else:
    pass

os.environ['GLTRACE_SIM_HOME'] = GLTRACE_SIM_HOME
os.environ['PATH']             = os.environ['PATH']
CMD                            = 'run-analyzer.sh'

args.output_dir = os.path.abspath(args.output_dir)

num_jobs = 0

for t in traces.traces:

  out_dir = os.path.join(args.output_dir, t.name)

  os.environ['OUT_DIR'] = out_dir

  if args.filter and args.filter not in t.name:
      print 'Skipping %s' % (t.name)
      continue

  print 'Simulating trace %s' % (t.name)     

  sbatch = 'sbatch --job-name %s --workdir %s -o %s -e %s' % (
      out_dir,
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

  os.environ['GLTRACE_SIM_BIN'] = 'gltracesim-analyze'

  os.environ['INPUT_DIR'] = os.path.abspath(
      os.path.join(args.input_dir, t.name)
  )

  os.environ['MODELS_FILE'] = os.path.abspath(args.models_file)

  os.environ['NUM_GPU_CORES'] = str(args.num_gpu_cores)

  if args.no_fast_forward:
      os.environ['F'] = '0'
  else:
      os.environ['F'] = str(t.fast_forward)

  if args.no_sim_end:
      os.environ['W'] = '0'
  else:
      os.environ['W'] = str(t.sim_end)
  

  os.system('%s %s' % (
      '/bin/bash' if not args.sbatch else sbatch,
      CMD
  )
  )

  num_jobs += 1

print "Submitted %d jobs." % (num_jobs)
