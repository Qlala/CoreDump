﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using CoreDumper;
using MyCoders;
namespace ConsoleApp1
{
    public partial class cdTestUIcs : Form
    {
        CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder> opener;
        long frame_size = 100000000;
        long last_frame=100;
        public void Interface_preset()
        {
            this.Addr_TrackBar.Maximum = (int)frame_size;
            this.Addr_TrackBar.Minimum = 0;
            this.Addr_TrackBar.TickFrequency = 10000;
            this.Frame_TrackBar.TickFrequency = 10000;
            this.Frame_TrackBar.Maximum = (int)last_frame ;
            this.Frame_TrackBar.Minimum = 0;
            this.Addr_TrackBar.Value = 0;
            this.Frame_TrackBar.Value = 0;
            this.Frame_TextBox.Text = "0";
            this.Addr_TextBox.Text = "0";
            this.Size_TextBox.Text = "8";
        }

        public cdTestUIcs(string file_to_open)
        {
            InitializeComponent();
            opener = new CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder>();
            opener.Decoder = new Deflate_Decoder();
            opener.readOnlyOpen(file_to_open);
            Interface_preset();
           
        }
        public async Task UpdataData_task()
        {
            UpdateData();
        }

        public void UpdateData()
        {
            long f = Int64.Parse(Frame_TextBox.Text);
            long pos = Int64.Parse(Addr_TextBox.Text);
            long size = Int64.Parse(Size_TextBox.Text);
            byte[] data = opener.randomAccesFrame(f, pos, size);
            DataView.Text = string.Join("", data);
            return;
        }


        private void Addr_Scroll(object sender, EventArgs e)
        {
            Addr_TextBox.Text =Addr_TrackBar.Value.ToString();
            if(AutoUpdate_Check.Checked)UpdateData();
        }
        private void Frame_Scroll(object sender, EventArgs e)
        {
            Frame_TextBox.Text= Frame_TrackBar.Value.ToString();
            if(AutoUpdate_Check.Checked)UpdateData();
        }
        private void Frame_Type(object sender, EventArgs e)
        {
            if (Frame_TextBox.Text != "")
            {
                Frame_TrackBar.Value = Int32.Parse(Frame_TextBox.Text);
            }
            if(AutoUpdate_Check.Checked)UpdateData();
        }
        private void Addr_Type(object sender, EventArgs e)
        {
            if (Addr_TextBox.Text!="")
            {
                Addr_TrackBar.Value = Int32.Parse(Addr_TextBox.Text);
            }
            if(AutoUpdate_Check.Checked)UpdateData();
        }

        private void cdTestUIcs_Load(object sender, EventArgs e)
        {

        }

        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void label3_Click(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
            UpdateData();
        }

        private void AutoUpdate_Check_CheckedChanged(object sender, EventArgs e)
        {

        }
    }
}
