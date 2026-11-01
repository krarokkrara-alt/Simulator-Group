using System;
using System.IO;
using System.Text;
using System.Threading;
using System.Management.Automation;
using System.Management.Automation.Runspaces;
using System.Windows.Forms;

internal static class ControlPanelLauncher
{
    [STAThread]
    private static int Main()
    {
        string baseDirectory = AppDomain.CurrentDomain.BaseDirectory;
        try
        {
            string scriptPath = Path.Combine(baseDirectory, "scripts", "Control-Panel.ps1");
            if (!File.Exists(scriptPath))
                throw new FileNotFoundException("Control panel script is missing. Keep the executable in the project folder.", scriptPath);

            // Hosting PowerShell inside this Windows application gives WinForms
            // a dedicated STA UI thread without opening a console window.
            using (Runspace runspace = RunspaceFactory.CreateRunspace())
            {
                runspace.ApartmentState = ApartmentState.STA;
                runspace.ThreadOptions = PSThreadOptions.UseCurrentThread;
                runspace.Open();
                runspace.SessionStateProxy.Path.SetLocation(baseDirectory);
                using (PowerShell shell = PowerShell.Create())
                {
                    shell.Runspace = runspace;
                    shell.AddScript(File.ReadAllText(scriptPath, Encoding.UTF8));
                    shell.Invoke();
                    if (shell.HadErrors)
                    {
                        StringBuilder messages = new StringBuilder();
                        foreach (ErrorRecord error in shell.Streams.Error)
                            messages.AppendLine(error.ToString());
                        throw new InvalidOperationException(messages.ToString());
                    }
                }
            }
            return 0;
        }
        catch (Exception error)
        {
            string logPath = Path.Combine(baseDirectory, "logs", "control-panel-error.log");
            string logStatus;
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(logPath));
                File.AppendAllText(logPath,
                    DateTimeOffset.Now.ToString("o") + Environment.NewLine + error +
                    Environment.NewLine + Environment.NewLine, Encoding.UTF8);
                logStatus = "Details: " + logPath;
            }
            catch (Exception logError)
            {
                logStatus = "Could not write error log: " + logError.Message;
            }
            MessageBox.Show("Control panel could not run.\n\n" + error.Message +
                "\n\n" + logStatus, "Mega 3ZZ Bench Control - Error",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
}
