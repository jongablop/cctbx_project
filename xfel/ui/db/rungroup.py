from __future__ import division
from xfel.ui.db import db_proxy

class Rungroup(db_proxy):
  def __init__(self, app, rungroup_id = None, **kwargs):
    db_proxy.__init__(self, app, "%s_rungroup" % app.params.experiment_tag, id = rungroup_id, **kwargs)
    self.rungroup_id = self.id

  def __getattr__(self, name):
    # Called only if the property cannot be found
    if name == "runs":
      startrun = self.app.get_run(run_number = self.startrun).run
      if self.endrun is None:
        return [r for r in self.app.get_all_runs() if r.run >= startrun]
      else:
        endrun = self.app.get_run(run_number = self.endrun).run
        return [r for r in self.app.get_all_runs() if r.run >= startrun and \
                                                      r.run <= endrun]
    else:
      return super(Rungroup, self).__getattr__(name)

  def __setattr__(self, name, value):
    assert name != "runs"
    super(Rungroup, self).__setattr__(name, value)

  def get_first_and_last_runs(self):
    runs = self.runs
    run_runs = [r.run for r in runs]
    first = runs[run_runs.index(min(run_runs))]
    if self.open:
      last = None
    else:
      last = runs[run_runs.index(max(run_runs))]
    return first, last

  def sync_runs(self, first_run, last_run):
    all_runs = self.app.get_all_runs()
    runs = self.runs
    run_numbers = [r.run for r in runs]
    if self.open:
      tester = lambda x: int(x.run) >= first_run # LCLS specific comparator
    else:
      tester = lambda x: int(x.run) >= first_run and int(x.run) <= last_run # LCLS specific comparator
    for run in all_runs:
      if tester(run):
        if not run.run in run_numbers:
          self.add_run(run.id)
      else:
        if run.run in run_numbers:
          self.remove_run(run.id)

  def add_run(self, run_id):
    query = "INSERT INTO `%s_rungroup_run` (rungroup_id, run_id) VALUES (%d, %d)" % ( \
      self.app.params.experiment_tag, self.id, run_id)
    self.app.execute_query(query, commit=True)

  def remove_run(self, run_id):
    query = "DELETE FROM `%s_rungroup_run` WHERE rungroup_id = %d AND run_id = %d" % ( \
      self.app.params.experiment_tag, self.id, run_id)
    self.app.execute_query(query, commit=True)
