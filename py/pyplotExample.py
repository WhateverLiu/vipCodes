

try:
  exec(open('py/rfuns.py').read())
except:
  exec(open('rfuns.py').read())
loadImage('pyplotExample.pickle')


if True:
  
  
  plt.rcParams["pdf.fonttype"] = 42
  tmp = "Times New Roman"
  if tmp not in pltFontManager.get_font_names(): tmp = 'C059'
  plt.rcParams["font.family"] = tmp
  del tmp
  plt.rcParams['axes.spines.bottom'] = True
  plt.rcParams['axes.spines.left'] = True
  plt.rcParams['axes.spines.right'] = False
  plt.rcParams['axes.spines.top'] = False
  plt.rcParams["figure.figsize"] = (10, 10 * 0.618)
  
  
  fig, axs = plt.subplot_mosaic([
     [0,   3,  7,   11],
     [1,   4,  8,  12],
     ['.', 5,  9,  '.'],
     [2,   6,  10,  13]
  ])
  
  
  # Plot Gaussian
  if True:
    tmp = axs[0].hist(jsamples[:, 0], bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[0].set_xlabel("$\\mu$", labelpad = 0, fontsize = 13)
    axs[0].set_ylabel("P", labelpad = 0, fontsize = 13)
    axs[0].set_title('Gaussian', fontsize = 13, pad = 0)
    
    
    tmp = axs[1].hist(jsamples[:, 1], bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[1].set_xlabel("$\\sigma$", fontsize = 13, labelpad = 0)
    
    
    trainPlot = train
    tmp = axs[2].hist(
      trainPlot, bins = 30, color = "red", density = True, alpha = 0.5, label ="Train")
    
    sp = seq(np.min(trainPlot), np.max(trainPlot), 100)
    p = ss.norm.pdf(sp, loc = gau['loc'], scale = gau['scale'])
    tmp = axs[2].plot(sp, p, color = "black", linewidth = 0.85, label = "Fitted")
    validPlot = valid
    tmp = axs[2].hist(
      validPlot, bins = 30, color = "blue", density = True, alpha =0.5, label = "Valid")
    axs[2].set_xlabel('Daily relative loss', fontsize = 13, labelpad = 0)  
    axs[2].legend(frameon = False, fontsize = 8)
  
  
  # Plot Gaussian mixture
  if True:
    axs[3].set_title('Gaussian mixture', fontsize = 13, pad = 0)
    axs[3].set_xlabel("$\\mu_1, \\mu_2$", labelpad = 0, fontsize = 13)
    tmp = axs[3].hist(jsamplesGM[:, 0], bins = 30, color = "green", density = True, alpha = 0.5)
    tmp = axs[3].hist(jsamplesGM[:, 2], bins = 30, color = "orange", density = True, alpha = 0.5)
    
    
    axs[4].set_xlabel("$\\sigma_1, \\sigma_2$", labelpad = 0, fontsize = 13)
    tmp = axs[4].hist(jsamplesGM[:, 1], bins = 30, color = "green", density = True, alpha = 0.5)
    tmp = axs[4].hist(jsamplesGM[:, 3], bins = 30, color = "orange", density = True, alpha = 0.5)
    
    
    axs[5].set_xlabel("$w$", labelpad = 0, fontsize = 13)
    tmp = axs[5].hist(jsamplesGM[:, 4], bins = 30, color = "gray", density = True, alpha = 0.5)
    
    
    trainPlot = train
    tmp = axs[6].hist(
      trainPlot, bins = 30, color = "red", density = True, alpha = 0.5, label ="Train")
    sp = seq(np.min(trainPlot), np.max(trainPlot), 100)
    p = ss.norm.pdf(sp, loc = gauMix['loc1'], scale = gauMix['scale1']) * gauMix['w'] + \
      ss.norm.pdf(sp, loc = gauMix['loc2'], scale = gauMix['scale2']) * (1 - gauMix['w'])
    tmp = axs[6].plot(sp, p, color = "black", linewidth = 0.85, label = "Fitted")
    
    
    validPlot = valid
    tmp = axs[6].hist(
      validPlot, bins = 30, color = "blue", density = True, alpha =0.5, label = "Valid")
    
  
  # Plot Student's t
  if True:
    tmp = axs[7].hist(jsamplesTstu[:, 0], bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[7].set_xlabel("$\\mu$", labelpad = 0, fontsize = 13)
    axs[7].set_title("Student's t", fontsize = 13, pad = 0)
    
    
    tmp = axs[8].hist(jsamplesTstu[:, 1], bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[8].set_xlabel("$\\sigma$", fontsize = 13, labelpad = 0)
    
    
    tmp = axs[9].hist(jsamplesTstu[:, 2], bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[9].set_xlabel("$\\nu$", fontsize = 13, labelpad = 0)
    
    
    trainPlot = train
    tmp = axs[10].hist(
      trainPlot, bins = 30, color = "red", density = True, alpha = 0.5, label ="Train")
    sp = seq(np.min(trainPlot), np.max(trainPlot), 100)
    p = ss.t.pdf(sp, loc = stu['loc'], scale = stu['scale'], df = stu['df'])
    tmp = axs[10].plot(sp, p, color = "black", linewidth = 0.85, label = "Fitted")
    validPlot = valid
    tmp = axs[10].hist(
      validPlot, bins = 30, color = "blue", density = True, alpha =0.5, label = "Valid")
      
      
  # Plot Gamma.
  if True:
    tmp = axs[11].hist(jsamplesGamma[:, 0], 
      bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[11].set_xlabel("$\\mu - x_{\\min}$", labelpad = 0, fontsize = 13)
    axs[11].set_title("Gamma", fontsize = 13, pad = 0)
    
    
    tmp = axs[12].hist(jsamplesGamma[:, 2], bins = 30, color = "gray", density = True, alpha = 0.5)
    axs[12].set_xlabel("$\\sigma$", fontsize = 13, labelpad = 0)
    
    
    trainPlot = train
    tmp = axs[13].hist(
      trainPlot, bins = 30, color = "red", density = True, alpha = 0.5, label ="Train")
    sp = seq(np.min(trainPlot), np.max(trainPlot), 100)
    p = ss.gamma.pdf(sp, a = ( (gam['mean'] - gam['shift']) / gam['sd']) ** 2, 
      scale = gam['sd'] ** 2 / (gam['mean'] - gam['shift']), loc = gam['shift'])
    tmp = axs[13].plot(sp, p, color = "black", linewidth = 0.85, label = "Fitted")
    validPlot = valid
    tmp = axs[13].hist(
      validPlot, bins = 30, color = "blue", density = True, alpha =0.5, label = "Valid")
    
  
  fig.subplots_adjust(wspace = 0.3, hspace = 0.5, bottom =  None )
  fig.savefig(figDir + '/' + symbolName + '.pdf', bbox_inches = 'tight', pad_inches = 0)









































