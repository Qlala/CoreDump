namespace ConsoleApp1
{
    partial class cdTestUIcs
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }



        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.Addr_TrackBar = new System.Windows.Forms.TrackBar();
            this.Addr_TextBox = new System.Windows.Forms.TextBox();
            this.Frame_TextBox = new System.Windows.Forms.TextBox();
            this.Frame_TrackBar = new System.Windows.Forms.TrackBar();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.Size_TextBox = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.DataView = new System.Windows.Forms.TextBox();
            this.AutoUpdate_Check = new System.Windows.Forms.CheckBox();
            this.button2 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.textBox1 = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.Addr_TrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.Frame_TrackBar)).BeginInit();
            this.SuspendLayout();
            // 
            // Addr_TrackBar
            // 
            this.Addr_TrackBar.BackColor = System.Drawing.SystemColors.Control;
            this.Addr_TrackBar.Location = new System.Drawing.Point(12, 161);
            this.Addr_TrackBar.Maximum = 1000;
            this.Addr_TrackBar.Name = "Addr_TrackBar";
            this.Addr_TrackBar.Size = new System.Drawing.Size(497, 45);
            this.Addr_TrackBar.TabIndex = 1;
            this.Addr_TrackBar.TickFrequency = 10000;
            this.Addr_TrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            this.Addr_TrackBar.Scroll += new System.EventHandler(this.Addr_Scroll);
            // 
            // Addr_TextBox
            // 
            this.Addr_TextBox.Location = new System.Drawing.Point(66, 132);
            this.Addr_TextBox.Name = "Addr_TextBox";
            this.Addr_TextBox.Size = new System.Drawing.Size(129, 20);
            this.Addr_TextBox.TabIndex = 2;
            this.Addr_TextBox.TextChanged += new System.EventHandler(this.Addr_Type);
            this.Addr_TextBox.Validated += new System.EventHandler(this.Addr_Type);
            // 
            // Frame_TextBox
            // 
            this.Frame_TextBox.Location = new System.Drawing.Point(53, 26);
            this.Frame_TextBox.Name = "Frame_TextBox";
            this.Frame_TextBox.Size = new System.Drawing.Size(129, 20);
            this.Frame_TextBox.TabIndex = 3;
            this.Frame_TextBox.TextChanged += new System.EventHandler(this.Frame_Type);
            this.Frame_TextBox.Validated += new System.EventHandler(this.Frame_Type);
            // 
            // Frame_TrackBar
            // 
            this.Frame_TrackBar.Location = new System.Drawing.Point(12, 67);
            this.Frame_TrackBar.Maximum = 1000;
            this.Frame_TrackBar.Name = "Frame_TrackBar";
            this.Frame_TrackBar.Size = new System.Drawing.Size(497, 45);
            this.Frame_TrackBar.TabIndex = 5;
            this.Frame_TrackBar.TickFrequency = 10000;
            this.Frame_TrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            this.Frame_TrackBar.Scroll += new System.EventHandler(this.Frame_Scroll);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(9, 29);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(42, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "Frame :";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 135);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(51, 13);
            this.label2.TabIndex = 7;
            this.label2.Text = "Address :";
            // 
            // Size_TextBox
            // 
            this.Size_TextBox.Location = new System.Drawing.Point(66, 183);
            this.Size_TextBox.Name = "Size_TextBox";
            this.Size_TextBox.Size = new System.Drawing.Size(100, 20);
            this.Size_TextBox.TabIndex = 8;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(13, 186);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(51, 13);
            this.label3.TabIndex = 9;
            this.label3.Text = "Data size";
            this.label3.Click += new System.EventHandler(this.label3_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(243, 183);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 10;
            this.button1.Text = "Update";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // DataView
            // 
            this.DataView.Location = new System.Drawing.Point(35, 265);
            this.DataView.Multiline = true;
            this.DataView.Name = "DataView";
            this.DataView.ReadOnly = true;
            this.DataView.Size = new System.Drawing.Size(369, 116);
            this.DataView.TabIndex = 11;
            // 
            // AutoUpdate_Check
            // 
            this.AutoUpdate_Check.AutoSize = true;
            this.AutoUpdate_Check.Location = new System.Drawing.Point(324, 186);
            this.AutoUpdate_Check.Name = "AutoUpdate_Check";
            this.AutoUpdate_Check.Size = new System.Drawing.Size(86, 17);
            this.AutoUpdate_Check.TabIndex = 12;
            this.AutoUpdate_Check.Text = "Auto Update";
            this.AutoUpdate_Check.UseVisualStyleBackColor = true;
            this.AutoUpdate_Check.CheckedChanged += new System.EventHandler(this.AutoUpdate_Check_CheckedChanged);
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(66, 227);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(92, 24);
            this.button2.TabIndex = 13;
            this.button2.Text = "Sauvegarder";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(259, 228);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(103, 23);
            this.button3.TabIndex = 14;
            this.button3.Text = "Performace test";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(368, 230);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(100, 20);
            this.textBox1.TabIndex = 15;
            this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
            // 
            // cdTestUIcs
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(516, 451);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.button3);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.AutoUpdate_Check);
            this.Controls.Add(this.DataView);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.Size_TextBox);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.Frame_TrackBar);
            this.Controls.Add(this.Frame_TextBox);
            this.Controls.Add(this.Addr_TextBox);
            this.Controls.Add(this.Addr_TrackBar);
            this.ForeColor = System.Drawing.SystemColors.ActiveCaption;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "cdTestUIcs";
            this.Text = "Core Dump Test UI";
            this.Load += new System.EventHandler(this.cdTestUIcs_Load);
            ((System.ComponentModel.ISupportInitialize)(this.Addr_TrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.Frame_TrackBar)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.TrackBar Addr_TrackBar;
        private System.Windows.Forms.TextBox Addr_TextBox;
        private System.Windows.Forms.TextBox Frame_TextBox;
        private System.Windows.Forms.TrackBar Frame_TrackBar;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox Size_TextBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.TextBox DataView;
        private System.Windows.Forms.CheckBox AutoUpdate_Check;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.TextBox textBox1;
    }
}