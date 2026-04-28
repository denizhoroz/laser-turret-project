
from packages.missions.m1 import Mission1
from packages.missions.m2 import Mission2

class SystemState:
    """Class to manage the state of the system."""
    
    def __init__(self):
        self.system_state = "idle"  # Possible states: idle, scanning, tracking, firing
        self.system_mission = "none"  # Possible missions: m1, m2, none
        self.system_data = {}  # Dictionary to hold any relevant data for the missions

    def set_state(self, new_state: str):
        """Sets the current state of the system."""
        valid_states = ["idle", "scanning", "tracking", "firing"]
        if new_state in valid_states:
            self.system_state = new_state
        else:
            raise ValueError(f"Invalid state: {new_state}. Valid states are: {valid_states}")
        
    def set_mission(self, new_mission: str):
        """Sets the current mission of the system."""
        valid_missions = ["m1", "m2", "none"]
        if new_mission in valid_missions:
            self.system_mission = new_mission
        else:
            raise ValueError(f"Invalid mission: {new_mission}. Valid missions are: {valid_missions}")
        
    def send_data(self, key: str, value):
        """Sends data to the system."""
        pass

    def start_mission(self, mission: str):
        """Starts a specific mission and updates the system state accordingly."""

        if self.system_mission != "none":
            print(f"Cannot start mission {mission} because mission {self.system_mission} is already active.")
            return
        
        self.set_mission(mission)
        if mission == "m1":
            Mission1(self).start()
        elif mission == "m2":
            Mission2(self).start()
        else:
            self.set_state("idle")

    def stop_mission(self):
        """Stops the current mission and resets the system state."""
        if self.system_mission == "none":
            print("No active mission to stop.")
            return
        
        print(f"Stopping mission {self.system_mission}.")
        self.set_mission("none")
        self.set_state("idle")