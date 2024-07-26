from .plan_manager import PlanManager

def cleanup_temporary_files(args):
    args.sas_file.unlink(missing_ok=True)
    PlanManager(args.plan_file).delete_existing_plans()
