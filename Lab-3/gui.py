import tkinter as tk
import subprocess
import threading

class ShellGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("LSH Terminal")
        self.root.geometry("750x500") # Give it a good starting size
        
        # --- Terminal Style Configurations ---
        BG_COLOR = "#1e1e1e"     # Dark grey/black background
        FG_COLOR = "#00ff00"     # Classic terminal green text
        FONT = ("Consolas", 12)  # Monospace font is crucial for terminal look
        
        self.root.configure(bg=BG_COLOR)

        # 1. Start the C program in the background
        self.process = subprocess.Popen(
            ['./main'], 
            stdin=subprocess.PIPE, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            text=True,
            bufsize=1
        )

        # 2. Setup Tkinter Widgets
        # Output Area
        self.output_area = tk.Text(
            root, 
            bg=BG_COLOR, 
            fg=FG_COLOR, 
            font=FONT, 
            state='disabled',
            bd=0,                  # Remove border
            highlightthickness=0,  # Remove focus outline
            wrap='word'
        )
        self.output_area.pack(expand=True, fill='both', padx=10, pady=(10, 0))

        # Input Area (Frame to hold the prompt symbol and entry side-by-side)
        self.input_frame = tk.Frame(root, bg=BG_COLOR)
        self.input_frame.pack(fill='x', padx=10, pady=(0, 10))

        self.prompt_label = tk.Label(
            self.input_frame, 
            text="lsh> ", 
            bg=BG_COLOR, 
            fg=FG_COLOR, 
            font=FONT
        )
        self.prompt_label.pack(side='left')

        self.input_entry = tk.Entry(
            self.input_frame, 
            bg=BG_COLOR, 
            fg=FG_COLOR, 
            font=FONT,
            bd=0, 
            highlightthickness=0,
            insertbackground=FG_COLOR # Make the blinking text cursor green too
        )
        self.input_entry.pack(side='left', expand=True, fill='x')
        self.input_entry.bind("<Return>", self.send_command)
        
        # Auto-focus the input box so you can type immediately
        self.input_entry.focus_set()
        
        # If the user clicks anywhere in the window, keep the focus on the input box
        self.root.bind("<Button-1>", lambda event: self.input_entry.focus_set())

        # 3. Start a background thread
        self.read_thread = threading.Thread(target=self.read_output, daemon=True)
        self.read_thread.start()

    def send_command(self, event):
        # Get text from the entry box and strip extra spaces
        command = self.input_entry.get().strip()
        self.input_entry.delete(0, tk.END)
        
        # --- INTERCEPT 'clear' COMMAND ---
        if command == "clear":
            self.output_area.config(state='normal')
            self.output_area.delete('1.0', tk.END) # Deletes all text
            self.output_area.config(state='disabled')
            return # Stop here so we don't send it to the C program
        
        if not command: # Ignore empty Enter presses
            return

        # Echo the typed command into the output area so it looks like a real shell history
        self.output_area.config(state='normal')
        self.output_area.insert(tk.END, f"lsh> {command}\n")
        self.output_area.config(state='disabled')
        self.output_area.see(tk.END)
        
        # Send it to the C program's stdin
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def read_output(self):
        # Constantly read from the C program's stdout and print to the Text widget
        for line in iter(self.process.stdout.readline, ''):
            
            # The C code automatically prints "> ". We strip it out here 
            # because we created our own persistent "lsh>" prompt in the UI.
            if line.startswith("> "):
                line = line[2:]
                
            if line.strip() or line == '\n': # Avoid printing entirely empty blank streams
                self.output_area.config(state='normal')
                self.output_area.insert(tk.END, line)
                self.output_area.config(state='disabled')
                self.output_area.see(tk.END)

if __name__ == "__main__":
    root = tk.Tk()
    app = ShellGUI(root)
    root.mainloop()