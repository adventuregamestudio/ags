using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Net;
using System.Web;
using System.Windows.Forms;

namespace AGS.Editor
{
	public partial class ExceptionDialog : Form
	{
        private Bitmap _screenShot;
        private Exception _exception;

		public ExceptionDialog(Exception ex, Bitmap screenShot)
		{
			InitializeComponent();

            _exception = ex;
            _screenShot = screenShot;

			txtErrorDetails.Text = "Error: " + ex.Message + Environment.NewLine +
				"Version: AGS " + AGS.Types.Version.AGS_EDITOR_VERSION + Environment.NewLine +
				Environment.NewLine + ex.ToString();
			txtErrorDetails.SelectionLength = 0;
			txtErrorDetails.SelectionStart = 0;
		}

        private void btnSendErrorReport_Click(object sender, EventArgs e)
        {
            try
            {
                string response = (string)BusyDialog.Show("Sending the error report to the AGS Website...", new BusyDialog.ProcessingHandler(SendErrorReportThread), null);
                if (!response.Contains("<ErrorReport>"))
                {
                    throw new AGS.Types.InvalidDataException("The server did not accept the error report.");
                }
                lblReportSucceeded.Visible = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to send the error report to the server. The AGS Website may be temporarily unavailable. Please copy and paste the error text to a thread in the AGS Technical Forum to report this problem." +
                    Environment.NewLine + Environment.NewLine + "Problem sending data: " + ex.Message, "Error reporting failed", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            btnSendErrorReport.Visible = false;
        }

        private string GetScreenshotAsSerializedBase64String()
        {
            if (_screenShot == null)
            {
                return string.Empty;
            }
            MemoryStream ms = new MemoryStream();
            _screenShot.Save(ms, System.Drawing.Imaging.ImageFormat.Png);
            byte[] serializedImage = ms.ToArray();
            return Convert.ToBase64String(serializedImage);
        }

        private object SendErrorReportThread(object parameter)
        {
            string exceptionText = "AGSVersion: " + AGS.Types.Version.AGS_EDITOR_VERSION +
                Environment.NewLine + "WinVer: " + Environment.OSVersion.VersionString +
                Environment.NewLine + _exception.ToString();

            string encodedForm = "exceptionText=" + HttpUtility.UrlEncode(exceptionText)
                + "&screenShot=" + HttpUtility.UrlEncode(GetScreenshotAsSerializedBase64String());

            byte[] dataToUpload = System.Text.Encoding.Default.GetBytes(encodedForm);

            using (System.Net.WebClient webClient = new System.Net.WebClient())
            {
                webClient.Headers.Add(HttpRequestHeader.ContentType, "application/x-www-form-urlencoded");
                byte[] reply = webClient.UploadData("http://www.bigbluecup.com/errorReport.php", dataToUpload);
                return System.Text.Encoding.Default.GetString(reply);
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
	}
}