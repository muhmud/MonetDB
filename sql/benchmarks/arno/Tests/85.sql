SELECT MODEL297.is_mutagen,MODEL297.lumo, count(distinct MODEL297.model_id ) FROM MODEL MODEL297  WHERE MODEL297.logp='1' group by MODEL297.lumo , MODEL297.is_mutagen;
